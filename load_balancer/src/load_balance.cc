#include "load_balance.h"
#include "globals.h"

uint32_t ssid;
std::shared_mutex ssid_mutex;
std::vector<uint64_t> timestamps;
std::vector<uint64_t> latency_results;

int
f_load_balance(__attribute__((unused)) void* arg)
{
    struct QueueConfig* queue_config = (struct QueueConfig*)arg;
    struct rte_mbuf* mbufs_recv[kRecvBurstSize];
    struct rte_mbuf* mbufs_send[kRecvBurstSize];
    uint32_t pkt_recv_num, pkt_handled_count;
    uint32_t pkt_sent_count, pkt_sent_num;
    bool pkt_has_sent = false, alloc_success;

    while (true) {
        pkt_recv_num = rte_eth_rx_burst(g_nic_info_data_plane.id, queue_config->queue_id, mbufs_recv, kRecvBurstSize);
        int alloc_success = rte_pktmbuf_alloc_bulk(queue_config->mempool_ptr, mbufs_send, pkt_recv_num);
        if (alloc_success != 0)
            rte_exit(EXIT_FAILURE, "rte_pktmbuf_alloc_bulk error\n");

        pkt_handled_count = 0;
        pkt_sent_count = 0;
        while (pkt_handled_count < pkt_recv_num) {
            pkt_has_sent =
                f_load_balance_pkt(mbufs_recv[pkt_handled_count], mbufs_send[pkt_sent_count], queue_config->queue_id);
            if (pkt_has_sent) {
                pkt_sent_count++;
                pkt_has_sent = false;
            }
            rte_pktmbuf_free(mbufs_recv[pkt_handled_count]);
            pkt_handled_count++;
        }

        if (pkt_sent_count > 0) {
            pkt_sent_num =
                rte_eth_tx_burst(g_nic_info_data_plane.id, queue_config->queue_id, mbufs_send, pkt_sent_count);
            if (pkt_sent_num < pkt_sent_count) {
                for (uint32_t i = pkt_sent_num; i < pkt_recv_num; i++)
                    rte_pktmbuf_free(mbufs_send[i]);
            }
        }
    }
    return 0;
}

bool
f_load_balance_pkt(struct rte_mbuf* recv_pkt, struct rte_mbuf* mbuf_send, uint16_t queue_id)
{
    uint32_t hdr_offset;
    struct rte_ether_hdr* eth_hdr;
    eth_hdr = rte_pktmbuf_mtod(recv_pkt, struct rte_ether_hdr*);
    std::vector<uint8_t> neighbor_mac(6);
    rte_memcpy(neighbor_mac.data(), eth_hdr->src_addr.addr_bytes, RTE_ETHER_ADDR_LEN);
    struct rte_ipv4_hdr* ip_hdr;
    hdr_offset = sizeof(struct rte_ether_hdr);
    ip_hdr = rte_pktmbuf_mtod_offset(recv_pkt, struct rte_ipv4_hdr*, hdr_offset);

    if (g_if_test_latency) {
        uint64_t recv_timestamp = f_clock_time_get();
        if (timestamps.size() % 2 == 0) {
            timestamps.push_back(recv_timestamp);
        } else {
            uint64_t last_timestamp = timestamps.back();
            uint64_t latency = recv_timestamp - last_timestamp;
            timestamps.push_back(recv_timestamp);
            latency_results.push_back(latency);
        }
        // TODO(lc): Make the hardcoded number of recorded results configurable.
        if (latency_results.size() % 100 == 0 && latency_results.size() > 0) {
            std::ofstream output_file("latency.txt", std::ios::app);
            for (uint32_t i = 0; i < latency_results.size(); i++) {
                output_file << latency_results[i] << "\n";
            }
            auto min_element = *std::min_element(latency_results.begin(), latency_results.end());
            output_file << "Minimum value: " << min_element << "\n";
            printf("Minimum value: %lu\n", min_element);
            output_file.close();
        }
    }
    // The locations of port in udp and tcp are same,
    // so only using udp header is enough.
    struct rte_udp_hdr* udp_hdr;
    hdr_offset += sizeof(struct rte_ipv4_hdr);
    udp_hdr = rte_pktmbuf_mtod_offset(recv_pkt, struct rte_udp_hdr*, hdr_offset);
    PackedFiveTupleKey five_tuple_key(
        ntohl(ip_hdr->src_addr),
        ntohl(ip_hdr->dst_addr),
        ip_hdr->next_proto_id,
        ntohs(udp_hdr->src_port),
        ntohs(udp_hdr->dst_port));
    struct PackedEndpoint endpoint = f_endpoint_hash(g_map_five_tuple_endpoint[queue_id], five_tuple_key);
    if ((ntohl(ip_hdr->src_addr) & g_virt_ip_mask) == g_virt_ip_prefix) {
        uint16_t pkt_ssid;
        pkt_ssid = ip_hdr->type_of_service + (ip_hdr->time_to_live >> 2) * 256;
        {
            std::unique_lock<std::shared_mutex> lock(ssid_mutex);
            uint32_t ssid_remainder = ssid % 16384;
            if (ssid_remainder <= 8192) {
                if (pkt_ssid > ssid_remainder && pkt_ssid < ssid_remainder + 8191) {
                    uint32_t new_ssid = pkt_ssid + 16384 * (ssid / 16384);
                    ssid = new_ssid;
                }
            } else {
                if (!(pkt_ssid > ssid_remainder - 8192 && pkt_ssid <= ssid_remainder)) {
                    uint32_t new_ssid = pkt_ssid + 16384 * (ssid / 16384);
                    if (pkt_ssid < ssid_remainder)
                        new_ssid += 16384;
                    ssid = new_ssid;
                }
            }
        }
    }
    if (!endpoint.is_zero()) {
        f_pkt_construct_data_plane(mbuf_send, neighbor_mac, (void*)eth_hdr, ntohs(ip_hdr->total_length) + 14, endpoint);
        return true;
    }
    return false;
}

int
f_ctrl_pkts_update(__attribute__((unused)) void* arg)
{
    struct QueueConfig* queue_config = (struct QueueConfig*)arg;
    struct rte_mbuf* mbufs_recv[kRecvBurstSize];
    struct rte_mbuf* mbufs_send[kRecvBurstSize];
    uint32_t pkt_recv_num, pkt_handled_count;
    uint32_t pkt_sent_count, pkt_sent_num;
    bool pkt_has_sent = false, alloc_success;

    while (true) {
        pkt_recv_num = rte_eth_rx_burst(g_nic_info_ctrl_plane.id, queue_config->queue_id, mbufs_recv, kRecvBurstSize);
        int alloc_success = rte_pktmbuf_alloc_bulk(queue_config->mempool_ptr, mbufs_send, pkt_recv_num);
        if (alloc_success != 0)
            rte_exit(EXIT_FAILURE, "rte_pktmbuf_alloc_bulk error\n");

        pkt_handled_count = 0;
        pkt_sent_count = 0;
        while (pkt_handled_count < pkt_recv_num) {
            pkt_has_sent = f_ctrl_pkt_handle(mbufs_recv[pkt_handled_count], mbufs_send[pkt_sent_count]);
            if (pkt_has_sent) {
                pkt_sent_count++;
                pkt_has_sent = false;
            }
            rte_pktmbuf_free(mbufs_recv[pkt_handled_count]);
            pkt_handled_count++;
        }

        if (pkt_sent_count > 0) {
            pkt_sent_num =
                rte_eth_tx_burst(g_nic_info_ctrl_plane.id, queue_config->queue_id, mbufs_send, pkt_sent_count);
            if (pkt_sent_num < pkt_sent_count) {
                for (uint32_t i = pkt_sent_num; i < pkt_recv_num; i++)
                    rte_pktmbuf_free(mbufs_send[i]);
            }
        }
    }
    return 0;
}

int
f_ctrl_pkt_handle(struct rte_mbuf* recv_pkt, struct rte_mbuf* mbuf_send)
{
    struct rte_ether_hdr* eth_hdr;
    eth_hdr = rte_pktmbuf_mtod(recv_pkt, struct rte_ether_hdr*);
    std::vector<uint8_t> neighbor_mac(6);
    rte_memcpy(neighbor_mac.data(), eth_hdr->src_addr.addr_bytes, RTE_ETHER_ADDR_LEN);
    if (eth_hdr->ether_type == htons(RTE_ETHER_TYPE_ARP)) {
        struct rte_arp_hdr* arp_hdr =
            rte_pktmbuf_mtod_offset(recv_pkt, struct rte_arp_hdr*, sizeof(struct rte_ether_hdr));
        uint32_t neighbor_ip = ntohl(arp_hdr->arp_data.arp_sip);
        uint32_t dst_ip = ntohl(arp_hdr->arp_data.arp_tip);
        if (dst_ip != g_nic_info_ctrl_plane.ip)
            return false;
        f_pkt_construct_arp_reply(mbuf_send, neighbor_mac, neighbor_ip);
        return true;
    }
    uint32_t hdr_offset = sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr);
    struct rte_udp_hdr* udp_hdr = rte_pktmbuf_mtod_offset(recv_pkt, struct rte_udp_hdr*, hdr_offset);
    hdr_offset += sizeof(struct rte_udp_hdr);
    void* data = rte_pktmbuf_mtod_offset(recv_pkt, void*, hdr_offset);
    uint8_t* request_type = (uint8_t*)data;
    // if (*request_type == 0) {
    //   struct PackedThreeTupleKey *three_tuple_data =
    //       (struct PackedThreeTupleKey *) (request_type + 1);
    //   struct PackedThreeTupleKey three_tuple_key(ntohl(three_tuple_data->ip),
    //                                              ntohs(three_tuple_data->port),
    //                                              three_tuple_data->protocol);
    //   struct PackedEndpoint *endpoint_data =
    //       (struct PackedEndpoint *) (three_tuple_data + 1);
    //   struct PackedEndpoint endpoint(ntohl(endpoint_data->ip),
    //                                  ntohs(endpoint_data->port));
    //   {
    //     std::unique_lock<std::shared_mutex> lock(g_lock_three_tuple_endpoints);
    //     auto iter = g_map_three_tuple_endpoints.find(three_tuple_key);
    //     if (iter == g_map_three_tuple_endpoints.end()) {
    //       std::vector<PackedEndpoint> endpoints;
    //       endpoints.push_back(endpoint);
    //       g_map_three_tuple_endpoints[three_tuple_key] = endpoints;
    //     }
    //     else {
    //       if (std::find(iter->second.begin(), iter->second.end(), endpoint) ==
    //           iter->second.end()) {
    //         iter->second.push_back(endpoint);
    //       }
    //     }
    //   }
    //   std::string response = "OK";
    //   struct PackedEndpoint controller(g_controller_ip,
    //                                     ntohs(udp_hdr->src_port));
    //   f_pkt_construct_ctrl_plane(mbuf_send, neighbor_mac,
    //                              (void*) &response, response.length(),
    //                              controller);
    //   return true;
    // }
    if (*request_type == 1) {
        uint32_t* new_ssid_ptr = (uint32_t*)(request_type + 1);
        uint32_t new_ssid = ntohl(*new_ssid_ptr);
        {
            std::unique_lock<std::shared_mutex> lock(ssid_mutex);
            if (new_ssid > ssid)
                ssid = new_ssid;
        }
        uint32_t ssid_reply = htonl(new_ssid);
        uint64_t recv_timestamp = f_clock_time_get();
        recv_timestamp = htobe64(recv_timestamp);
        unsigned char data[sizeof(ssid_reply) + sizeof(recv_timestamp)];
        memcpy(data, &ssid_reply, sizeof(ssid_reply));
        memcpy(data + sizeof(ssid_reply), &recv_timestamp, sizeof(recv_timestamp));
        struct PackedEndpoint controller(g_controller_ip, ntohs(udp_hdr->dst_port));
        f_pkt_construct_ctrl_plane(mbuf_send, neighbor_mac, data, sizeof(data), controller);
        return true;
    } else if (*request_type == 2) {
        struct PackedThreeTupleKey* three_tuple_data = (struct PackedThreeTupleKey*)(request_type + 1);
        struct PackedThreeTupleKey three_tuple_key(
            ntohl(three_tuple_data->ip), ntohs(three_tuple_data->port), three_tuple_data->protocol);
        struct PackedEndpoint* endpoint_data = (struct PackedEndpoint*)(three_tuple_data + 1);
        struct PackedEndpoint endpoint(ntohl(endpoint_data->ip), ntohs(endpoint_data->port));
        uint32_t* label_ptr = (uint32_t*)(endpoint_data + 1);
        {
            std::unique_lock<std::shared_mutex> lock(g_lock_three_tuple_endpoints);
            auto iter = g_map_three_tuple_endpoints.find(three_tuple_key);
            if (iter == g_map_three_tuple_endpoints.end()) {
                std::vector<PackedEndpoint> endpoints;
                endpoints.push_back(endpoint);
                g_map_three_tuple_endpoints[three_tuple_key] = endpoints;
            } else {
                if (std::find(iter->second.begin(), iter->second.end(), endpoint) == iter->second.end()) {
                    iter->second.push_back(endpoint);
                }
            }
        }
        uint32_t label = ntohl(*label_ptr);
        struct PackedEndpoint controller(g_controller_ip, ntohs(udp_hdr->src_port));
        f_pkt_construct_ctrl_plane(mbuf_send, neighbor_mac, (void*)&label, sizeof(label), controller);
        return true;
    }
    return false;
}

void
f_pkt_construct_arp_reply(struct rte_mbuf* mbuf, const std::vector<uint8_t>& dst_mac, uint32_t dst_ip)
{
    uint32_t total_pkt_len = sizeof(struct rte_ether_hdr) + sizeof(struct rte_arp_hdr);
    mbuf->pkt_len = total_pkt_len;
    mbuf->data_len = total_pkt_len;
    uint8_t* data = rte_pktmbuf_mtod(mbuf, uint8_t*);
    struct rte_ether_hdr* eth_hdr = (struct rte_ether_hdr*)data;
    rte_memcpy(eth_hdr->src_addr.addr_bytes, &g_nic_info_ctrl_plane.mac[0], RTE_ETHER_ADDR_LEN);
    rte_memcpy(eth_hdr->dst_addr.addr_bytes, &dst_mac[0], RTE_ETHER_ADDR_LEN);
    eth_hdr->ether_type = htons(RTE_ETHER_TYPE_ARP);

    struct rte_arp_hdr* arp_hdr = (struct rte_arp_hdr*)(eth_hdr + 1);
    arp_hdr->arp_hardware = htons(1);
    arp_hdr->arp_protocol = htons(RTE_ETHER_TYPE_IPV4);
    arp_hdr->arp_hlen = RTE_ETHER_ADDR_LEN;
    arp_hdr->arp_plen = sizeof(uint32_t);
    arp_hdr->arp_opcode = htons(RTE_ARP_OP_REPLY);
    rte_memcpy(arp_hdr->arp_data.arp_sha.addr_bytes, &g_nic_info_ctrl_plane.mac[0], RTE_ETHER_ADDR_LEN);
    rte_memcpy(arp_hdr->arp_data.arp_tha.addr_bytes, &dst_mac[0], RTE_ETHER_ADDR_LEN);
    arp_hdr->arp_data.arp_sip = htonl(g_nic_info_ctrl_plane.ip);
    arp_hdr->arp_data.arp_tip = htonl(dst_ip);
}

void
f_pkt_construct_data_plane(
    struct rte_mbuf* mbuf,
    const std::vector<uint8_t>& neighbor_mac,
    void* data,
    uint32_t data_length,
    const struct PackedEndpoint& endpoint)
{
    uint32_t total_pkt_len = data_length + kUdpHdrSize + 4;
    mbuf->pkt_len = total_pkt_len;
    mbuf->data_len = total_pkt_len;
    uint8_t* pkt_data = rte_pktmbuf_mtod(mbuf, uint8_t*);

    struct rte_ether_hdr* eth_hdr = (struct rte_ether_hdr*)pkt_data;
    rte_memcpy(eth_hdr->src_addr.addr_bytes, g_nic_info_data_plane.mac.data(), RTE_ETHER_ADDR_LEN);
    rte_memcpy(eth_hdr->dst_addr.addr_bytes, neighbor_mac.data(), RTE_ETHER_ADDR_LEN);

    eth_hdr->ether_type = htons(RTE_ETHER_TYPE_IPV4);

    uint32_t hdr_offset = sizeof(struct rte_ether_hdr);
    struct rte_ipv4_hdr* ip_hdr = (struct rte_ipv4_hdr*)(pkt_data + hdr_offset);
    ip_hdr->version_ihl = 0x45;
    ip_hdr->type_of_service = 0;
    ip_hdr->total_length = htons(total_pkt_len - hdr_offset);
    ip_hdr->packet_id = 0;
    ip_hdr->fragment_offset = 0;
    ip_hdr->time_to_live = 64;
    ip_hdr->next_proto_id = IPPROTO_UDP;
    ip_hdr->src_addr = htonl(g_nic_info_data_plane.ip);
    ip_hdr->dst_addr = htonl(endpoint.ip);
    ip_hdr->hdr_checksum = 0;
    ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);

    hdr_offset += sizeof(struct rte_ipv4_hdr);
    struct rte_udp_hdr* udp_hdr = (struct rte_udp_hdr*)(pkt_data + hdr_offset);
    udp_hdr->src_port = htons(g_local_port_data_plane);
    udp_hdr->dst_port = htons(endpoint.port);
    uint16_t udp_len = total_pkt_len - hdr_offset;
    udp_hdr->dgram_len = htons(udp_len);

    hdr_offset += sizeof(struct rte_udp_hdr);
    uint32_t* ssid_ptr = (uint32_t*)(pkt_data + hdr_offset);
    {
        std::shared_lock<std::shared_mutex> lock(ssid_mutex);
        *ssid_ptr = htonl(ssid);
    }
    rte_memcpy((uint8_t*)(ssid_ptr + 1), (uint8_t*)data, data_length);
    udp_hdr->dgram_cksum = 0;
    udp_hdr->dgram_cksum = rte_ipv4_udptcp_cksum(ip_hdr, udp_hdr);
}

void
f_pkt_construct_ctrl_plane(
    struct rte_mbuf* mbuf,
    const std::vector<uint8_t>& neighbor_mac,
    void* data,
    uint32_t data_length,
    const struct PackedEndpoint& endpoint)
{
    uint32_t total_pkt_len = data_length + kUdpHdrSize;
    mbuf->pkt_len = total_pkt_len;
    mbuf->data_len = total_pkt_len;
    uint8_t* pkt_data = rte_pktmbuf_mtod(mbuf, uint8_t*);

    struct rte_ether_hdr* eth_hdr = (struct rte_ether_hdr*)pkt_data;
    rte_memcpy(eth_hdr->src_addr.addr_bytes, g_nic_info_ctrl_plane.mac.data(), RTE_ETHER_ADDR_LEN);
    rte_memcpy(eth_hdr->dst_addr.addr_bytes, neighbor_mac.data(), RTE_ETHER_ADDR_LEN);
    eth_hdr->ether_type = htons(RTE_ETHER_TYPE_IPV4);

    uint32_t hdr_offset = sizeof(struct rte_ether_hdr);
    struct rte_ipv4_hdr* ip_hdr = (struct rte_ipv4_hdr*)(pkt_data + hdr_offset);
    ip_hdr->version_ihl = 0x45;
    ip_hdr->type_of_service = 0;
    ip_hdr->total_length = htons(total_pkt_len - hdr_offset);
    ip_hdr->packet_id = 0;
    ip_hdr->fragment_offset = 0;
    ip_hdr->time_to_live = 64;
    ip_hdr->next_proto_id = IPPROTO_UDP;
    ip_hdr->src_addr = htonl(g_nic_info_ctrl_plane.ip);
    ip_hdr->dst_addr = htonl(endpoint.ip);
    ip_hdr->hdr_checksum = 0;
    ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);

    hdr_offset += sizeof(rte_ipv4_hdr);
    struct rte_udp_hdr* udp_hdr = (struct rte_udp_hdr*)(pkt_data + hdr_offset);
    udp_hdr->src_port = htons(g_local_port_ctrl_plane);
    udp_hdr->dst_port = htons(endpoint.port);
    uint16_t udp_len = total_pkt_len - hdr_offset;
    udp_hdr->dgram_len = htons(udp_len);
    rte_memcpy((uint8_t*)(udp_hdr + 1), (uint8_t*)data, data_length);
    udp_hdr->dgram_cksum = 0;
    udp_hdr->dgram_cksum = rte_ipv4_udptcp_cksum(ip_hdr, udp_hdr);
}

uint64_t
f_clock_time_get()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)(ts.tv_sec) * 1e9 + ts.tv_nsec;
}
