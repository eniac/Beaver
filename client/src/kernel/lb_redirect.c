#include "lb_redirect.h"

/*
* @brief: Map the load balancers' index to the load balancer ip,
          store the number of load balancers in the last index (Default 1023).
* @format: {lb_index: lb_ip (network_order)}
*/
struct bpf_map_def SEC("maps") lb_ip_array = {
    .type = BPF_MAP_TYPE_ARRAY,
    .key_size = sizeof(__u32),
    .value_size = sizeof(__u32),
    .max_entries = kArraySize,
};

/*
* @brief: Map the hash value of ports to virtual ip,
          using for snat when getting response from backends.
* @format: {port_hash_value: virtual_ip (network_order)}
*/
struct bpf_map_def SEC("maps") virt_ip_hash_table = {
    .type = BPF_MAP_TYPE_HASH,
    .key_size = sizeof(__u32),
    .value_size = sizeof(__u32),
    .max_entries = kHashTableSize,
};

/*
 * @brief: Hash by 5-tuple to get destination load balancer index,
 *         then redirect the packet there.
 */
SEC("tc_egress_filter")
int
f_lb_redirect(struct __sk_buff* skb)
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
        // The port locations of tcp and udp are the same, so udp applies for both.
        if (ip_hdr.protocol == IPPROTO_UDP || ip_hdr.protocol == IPPROTO_TCP) {
            __u32 last_index = kArraySize - 1;
            __u32* lb_number = bpf_map_lookup_elem(&lb_ip_array, &last_index);
            if (!lb_number)
                return BPF_DROP;

            struct udphdr udp_hdr;
            if (bpf_skb_load_bytes(skb, offset, &udp_hdr, sizeof(udp_hdr)) < 0)
                return BPF_DROP;
            if (bpf_ntohs(udp_hdr.dest) >= kDstPortRedirect) {
                __u32 lb_index = f_lb_index_hash(
                    bpf_ntohl(ip_hdr.saddr),
                    bpf_ntohl(ip_hdr.daddr),
                    bpf_ntohs(udp_hdr.source),
                    bpf_ntohs(udp_hdr.dest),
                    ip_hdr.protocol,
                    *lb_number);
                __u32* lb_ip = bpf_map_lookup_elem(&lb_ip_array, &lb_index);
                if (!lb_ip)
                    return BPF_DROP;
                __u32 virt_ip = ip_hdr.daddr;
                ip_hdr.daddr = *lb_ip;
                bpf_skb_store_bytes(skb, offset, &ip_hdr, sizeof(ip_hdr), 0);
                __u32 port_hash_key = f_port_hash_key_get(udp_hdr.source, udp_hdr.dest);
                bpf_map_update_elem(&virt_ip_hash_table, &port_hash_key, &virt_ip, BPF_ANY);
                return BPF_OK;
            }
        }
    }
    return BPF_OK;
}

char _license[] SEC("license") = "GPL";