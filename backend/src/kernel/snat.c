#include "nat.h"

/*
 * @brief: Hash table to search the virtual information for snat
 *         based on the port and protocol.
 */
struct bpf_map_def SEC("maps") snat_info = {
    .type = BPF_MAP_TYPE_HASH,
    .key_size = sizeof(struct PackedPortProto),
    .value_size = sizeof(struct PackedVirtInfo),
    .max_entries = 1024,
};

/*
 * @brief: Array to store the information about snapshot(ss) on the node.
 *         Index 0 (is_ss): 1 means snapshot is enabled, 0 means disabled.
 *         Index 1 (ssid): snapshot id.
 */
struct bpf_map_def SEC("maps") ss_info = {
    .type = BPF_MAP_TYPE_ARRAY,
    .key_size = sizeof(__u32),
    .value_size = sizeof(__u32),
    .max_entries = 4,
};

/*
 * @brief: Queue that stores requests for virtual information from manager.
 *         There are requests when the backend reach out to others at first.
 */
struct bpf_map_def SEC("maps") virt_info_reqs = {
    .type = BPF_MAP_TYPE_QUEUE,
    .key_size = 0,
    .value_size = sizeof(struct PackedVirtInfoReq),
    .max_entries = 1024,
};

/*
 * @brief: Store the existing status of the requests.
 */
struct bpf_map_def SEC("maps") existing_reqs = {
    .type = BPF_MAP_TYPE_HASH,
    .key_size = sizeof(struct PackedVirtInfoReq),
    .value_size = sizeof(__u32),
    .max_entries = 1024,
};

/*
 * @brief: Load the ssid into packet header.
 *         Use unused part in ip header to store ssid.
 */
int
f_ssid_load(struct iphdr* ip_hdr)
{
    __u32 ssid_key = 1;
    __u32* ssid = bpf_map_lookup_elem(&ss_info, &ssid_key);
    if (ssid) {
        __u32 ssid_14bit = (*ssid) % 16384;
        // Use tos and the top six bits of ttl to store ssid.
        ip_hdr->tos = ssid_14bit % 256;
        ip_hdr->ttl = ((ssid_14bit / 256) << 2) + 3;
        return 0;
    } else
        return -1;
}

/*
 * @brief: Load the virtual information into packet header.
 */
int
f_virt_info_load(struct iphdr* ip_hdr, struct udphdr* udp_hdr)
{
    struct PackedPortProto port_proto = {.port = udp_hdr->source, .protocol = ip_hdr->protocol};
    struct PackedVirtInfo* virt_info_ptr = bpf_map_lookup_elem(&snat_info, &port_proto);
    if (!virt_info_ptr) {
        struct PackedVirtInfoReq virt_info_req = {
            .ip = ip_hdr->saddr,
            .port = udp_hdr->source,
            .protocol = ip_hdr->protocol,
            .client_ip = ip_hdr->daddr,
            .client_port = udp_hdr->dest};
        __u32* existing = bpf_map_lookup_elem(&existing_reqs, &virt_info_req);
        if (!existing) {
            __u32 status = 1;
            bpf_map_update_elem(&existing_reqs, &virt_info_req, &status, BPF_NOEXIST);
            bpf_map_push_elem(&virt_info_reqs, &virt_info_req, 0);
        }
        return -1;
    } else {
        ip_hdr->saddr = virt_info_ptr->virt_ip;
        udp_hdr->source = virt_info_ptr->virt_port;
        return 0;
    }
}

/*
 * @brief: Function hooked at tc mainly for snat.
 *         (Change the physical [ip, port] to virtual [ip, port]).
 */
SEC("tc_egress_filter")
int
f_snat(struct __sk_buff* skb)
{
    struct ethhdr eth_hdr;
    int offset = sizeof(struct ethhdr);
    if (bpf_skb_load_bytes(skb, 0, &eth_hdr, sizeof(eth_hdr)) < 0)
        return BPF_DROP;

    if (eth_hdr.h_proto == bpf_htons(ETH_P_IP)) {
        struct iphdr ip_hdr;
        if (bpf_skb_load_bytes(skb, offset, &ip_hdr, sizeof(ip_hdr)) < 0)
            return BPF_DROP;
        offset += sizeof(ip_hdr);
        // The location of port is the same for tcp and udp headers.
        if (ip_hdr.protocol == kProtoUdp || ip_hdr.protocol == kProtoTcp) {
            struct udphdr udp_hdr;
            if (bpf_skb_load_bytes(skb, offset, &udp_hdr, sizeof(udp_hdr)) < 0)
                return BPF_DROP;

            if (udp_hdr.dest == bpf_htons(kIntraServicePort) || udp_hdr.source == bpf_htons(kIntraServicePort)) {
                if (f_ssid_load(&ip_hdr) < 0)
                    return BPF_DROP;
                bpf_skb_store_bytes(skb, sizeof(eth_hdr), &ip_hdr, sizeof(ip_hdr), 0);
                return BPF_OK;
            }

            if (f_virt_info_load(&ip_hdr, &udp_hdr) < 0)
                return BPF_DROP;
            bpf_skb_store_bytes(skb, sizeof(eth_hdr), &ip_hdr, sizeof(ip_hdr), 0);
            bpf_skb_store_bytes(skb, sizeof(eth_hdr) + sizeof(ip_hdr), &udp_hdr, sizeof(udp_hdr), 0);

            if ((bpf_ntohl(ip_hdr.daddr) & kVirtIpMask) == kVirtIpPrefix) {
                if (f_ssid_load(&ip_hdr) < 0)
                    return BPF_DROP;
                bpf_skb_store_bytes(skb, sizeof(eth_hdr), &ip_hdr, sizeof(ip_hdr), 0);
                return BPF_OK;
            }
            return BPF_OK;
        }
        return BPF_OK;
    }
    return BPF_OK;
}

char _license[] SEC("license") = "GPL";