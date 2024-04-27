#include "lb_redirect.h"

/*
* @brief: Map the hash value of ports to virtual ip,
          using for snat when getting response from backends.
* @format: {port_hash_value: virtual_ip}
*/
struct bpf_map_def SEC("maps") virt_ip_hash_table = {
    .type = BPF_MAP_TYPE_HASH,
    .key_size = sizeof(__u32),
    .value_size = sizeof(__u32),
    .max_entries = kHashTableSize,
};

/*
 * @brief: Transfer the physical ip into virtual ip
 */
SEC("xdp_ingress_filter")
int
f_client_snat(struct xdp_md* ctx)
{
    struct ethhdr* eth_hdr = (struct ethhdr*)(long)ctx->data;
    if ((void*)(eth_hdr + 1) > (void*)(long)ctx->data_end)
        return XDP_PASS;
    struct iphdr* ip_hdr = (struct iphdr*)(eth_hdr + 1);
    if ((void*)(ip_hdr + 1) > (void*)(long)ctx->data_end)
        return XDP_PASS;
    struct udphdr* udp_hdr = (struct udphdr*)(ip_hdr + 1);
    if ((void*)(udp_hdr + 1) > (void*)(long)ctx->data_end)
        return XDP_PASS;

    if (bpf_ntohs(udp_hdr->source) >= kDstPortRedirect) {
        __u32 port_hash_key = f_port_hash_key_get(udp_hdr->dest, udp_hdr->source);
        __u32* virt_ip = bpf_map_lookup_elem(&virt_ip_hash_table, &port_hash_key);
        if (!virt_ip) {
            __u32 old_src_ip = ip_hdr->saddr;
            ip_hdr->saddr = *virt_ip;
            __u16 new_checksum = f_csum_update_for_ip(old_src_ip, *virt_ip, ip_hdr->check);
            ip_hdr->check = new_checksum;
        }
    }
    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";