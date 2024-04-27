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

struct bpf_map_def SEC("maps") bot_record = {
    .type = BPF_MAP_TYPE_RINGBUF,
    .max_entries = 1024 * 1024,
};

/*
 * @brief: Compute ssid stored in tcp packet.
 *         Tcp packet represents the flow from other backends (Intra service).
 */
int
f_ssid_get_tcp(struct xdp_md* ctx, struct ethhdr* eth_hdr, struct iphdr* ip_hdr)
{
    struct tcphdr* tcp_hdr = (struct tcphdr*)(ip_hdr + 1);
    if ((void*)(tcp_hdr + 1) > (void*)(long)ctx->data_end)
        return XDP_PASS;

    if (tcp_hdr->dest == bpf_htons(kIntraServicePort) || tcp_hdr->source == bpf_htons(kIntraServicePort)) {
        __u32 pkt_ssid = ip_hdr->tos + (ip_hdr->ttl >> 2) * 256;
        __u32 ssid_key = 1;
        __u32* ssid = bpf_map_lookup_elem(&ss_info, &ssid_key);
        if (ssid) {
            __u32 ssid_remainder = (*ssid) % 16384;
            // Only allow maximum 8192 fly snapshot number.
            if (ssid_remainder <= 8192) {
                if (pkt_ssid > ssid_remainder && pkt_ssid <= ssid_remainder + 8191) {
                    __u32 new_ssid = pkt_ssid + 16384 * ((*ssid) / 16384);
                    bpf_map_update_elem(&ss_info, &ssid_key, &new_ssid, BPF_ANY);
                    return XDP_PASS;
                }
            } else {
                if (!(pkt_ssid > ssid_remainder - 8192 && pkt_ssid <= ssid_remainder)) {
                    __u32 new_ssid = pkt_ssid + 16384 * ((*ssid) / 16384);
                    if (pkt_ssid < ssid_remainder)
                        new_ssid += 16384;
                    bpf_map_update_elem(&ss_info, &ssid_key, &new_ssid, BPF_ANY);
                    return XDP_PASS;
                }
            }
        }
        return XDP_PASS;
    }
    return XDP_PASS;
}

/*
 * @brief: The main realization for nat in udp packet from load balancer.
 */
int
f_nat_udp(struct xdp_md* ctx, struct ethhdr* eth_hdr, struct iphdr* ip_hdr)
{
    struct udphdr* udp_hdr = (struct udphdr*)(ip_hdr + 1);
    if ((void*)(udp_hdr + 1) > (void*)(long)ctx->data_end)
        return XDP_PASS;

    __u32* pkt_ssid = (__u32*)(udp_hdr + 1);
    if ((void*)(pkt_ssid + 1) > (void*)(long)ctx->data_end)
        return XDP_PASS;
    __u32 pkt_ssid_host = bpf_ntohl(*pkt_ssid);
    __u32 ssid_key = 1;
    __u32* ssid = bpf_map_lookup_elem(&ss_info, &ssid_key);
    __u32 old_ssid = 0;
    if (ssid) {
        old_ssid = *ssid;
        if (*ssid < pkt_ssid_host)
            bpf_map_update_elem(&ss_info, &ssid_key, &pkt_ssid_host, BPF_ANY);
    } else
        return XDP_PASS;

    struct ethhdr* inner_eth_hdr = (struct ethhdr*)(pkt_ssid + 1);
    if ((void*)(inner_eth_hdr + 1) > (void*)(long)ctx->data_end)
        return XDP_PASS;
    __builtin_memcpy(inner_eth_hdr, eth_hdr, sizeof(struct ethhdr));

    struct iphdr* inner_ip_hdr = (struct iphdr*)(inner_eth_hdr + 1);
    if ((void*)(inner_ip_hdr + 1) > (void*)(long)ctx->data_end)
        return XDP_PASS;
    __u32 old_ip = inner_ip_hdr->daddr;
    __u32 new_ip = ip_hdr->daddr;
    __u32 old_src_ip = inner_ip_hdr->saddr;
    __u16 new_checksum = f_csum_update_for_ip(old_ip, new_ip, inner_ip_hdr->check);
    inner_ip_hdr->daddr = ip_hdr->daddr;
    inner_ip_hdr->check = new_checksum;

    __u16 old_port, new_port;
    struct PackedPortProto snat_key = {.port = udp_hdr->dest, .protocol = inner_ip_hdr->protocol};
    if (inner_ip_hdr->protocol == kProtoUdp) {
        struct udphdr* inner_udp_hdr = (struct udphdr*)(inner_ip_hdr + 1);
        if ((void*)(inner_udp_hdr + 1) > (void*)(long)ctx->data_end)
            return XDP_PASS;
        __u32 if_bot_key = 3;
        __u32* if_bot_value = bpf_map_lookup_elem(&ss_info, &if_bot_key);
        if (if_bot_value && *if_bot_value == 1) {
            // __u32 bot_key = 2;
            // __u32 *bot_value = bpf_map_lookup_elem(&ss_info, &bot_key);
            // if (bot_value) {
            // __u32 bot_record_value = *bot_value;
            if (old_ssid < pkt_ssid_host) {
                void* bot_data = bpf_ringbuf_reserve(&bot_record, sizeof(__u32), 0);
                if (bot_data) {
                    *(__u32*)bot_data = pkt_ssid_host;
                    bpf_ringbuf_submit(bot_data, 0);
                }
            }
            // __sync_fetch_and_add(bot_value, 1);
            // }
        }
        new_checksum = f_csum_update_for_ip(old_ip, new_ip, inner_udp_hdr->check);
        new_checksum = f_csum_update_for_port(inner_udp_hdr->dest, udp_hdr->dest, new_checksum);
        old_port = inner_udp_hdr->dest;
        new_port = udp_hdr->dest;
        inner_udp_hdr->dest = udp_hdr->dest;
        inner_udp_hdr->check = new_checksum;
    } else if (inner_ip_hdr->protocol == kProtoTcp) {
        struct tcphdr* inner_tcp_hdr = (struct tcphdr*)(inner_ip_hdr + 1);
        if ((void*)(inner_tcp_hdr + 1) > (void*)(long)ctx->data_end)
            return XDP_PASS;
        new_checksum = f_csum_update_for_ip(old_ip, new_ip, inner_tcp_hdr->check);
        new_checksum = f_csum_update_for_port(inner_tcp_hdr->dest, udp_hdr->dest, new_checksum);
        old_port = inner_tcp_hdr->dest;
        new_port = udp_hdr->dest;
        inner_tcp_hdr->dest = udp_hdr->dest;
        inner_tcp_hdr->check = new_checksum;
    } else
        return XDP_PASS;

    struct PackedVirtInfo* snat_virt_info = bpf_map_lookup_elem(&snat_info, &snat_key);
    if (!snat_virt_info) {
        struct PackedVirtInfo new_virt_info = {.virt_ip = old_ip, .virt_port = old_port};
        bpf_map_update_elem(&snat_info, &snat_key, &new_virt_info, BPF_ANY);
    }

    if (bpf_xdp_adjust_head(ctx, 46))
        return XDP_DROP;
    return XDP_PASS;
}

/*
 * @brief: Function hooked at xdp mainly for nat.
 *         (Change the virtual [ip, port] to physical [ip, port]).
 */
SEC("xdp_ingress_filter")
int
f_nat(struct xdp_md* ctx)
{
    struct ethhdr* eth_hdr = (struct ethhdr*)(long)ctx->data;
    if ((void*)(eth_hdr + 1) > (void*)(long)ctx->data_end)
        return XDP_PASS;
    struct iphdr* ip_hdr = (struct iphdr*)(eth_hdr + 1);
    if ((void*)(ip_hdr + 1) > (void*)(long)ctx->data_end)
        return XDP_PASS;

    if (ip_hdr->protocol == kProtoTcp)
        return f_ssid_get_tcp(ctx, eth_hdr, ip_hdr);
    else if (ip_hdr->protocol == kProtoUdp)
        return f_nat_udp(ctx, eth_hdr, ip_hdr);
    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";