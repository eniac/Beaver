#include "virt_info_manage.h"
#include "globals.h"

std::queue<uint32_t> req_labels;
std::set<std::pair<uint32_t, uint16_t>> used_virt_info;
std::unordered_map<uint32_t, uint32_t> req_label_reply_count;
std::unordered_map<uint32_t, struct PackedEndpoint> req_label_src_info;
std::unordered_map<uint32_t, struct PackedVirtInfo> req_label_virt_info;

struct PackedVirtInfo
f_virt_info_generate()
{
    struct PackedVirtInfo virt_info;
    std::srand(std::time(nullptr));
    while (true) {
        uint8_t third_byte = 100;
        uint8_t fourth_byte = 100 + std::rand() % 156;
        virt_info.virt_ip =
            inet_addr(("192.168." + std::to_string(third_byte) + "." + std::to_string(fourth_byte)).c_str());
        virt_info.virt_port = kMinVirtPort + std::rand() % (65536 - kMinVirtPort);
        uint32_t used_ip = virt_info.virt_ip;
        uint16_t used_port = virt_info.virt_port;
        if (used_virt_info.find({used_ip, used_port}) == used_virt_info.end()) {
            used_virt_info.insert({used_ip, used_port});
            break;
        }
    }
    return virt_info;
}

void
f_pkt_construct(
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
    rte_memcpy(eth_hdr->src_addr.addr_bytes, &g_nic_info_ctrl_plane.mac[0], RTE_ETHER_ADDR_LEN);
    rte_memcpy(eth_hdr->dst_addr.addr_bytes, &neighbor_mac[0], RTE_ETHER_ADDR_LEN);
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

int
f_virt_info_reqs_manage(__attribute__((unused)) void* arg)
{
    struct QueueConfig* queue_config = (struct QueueConfig*)arg;
    struct rte_mbuf* mbufs_recv[kRecvBurstSize];
    uint32_t pkt_recv_num;
    uint32_t src_ip;
    struct rte_ipv4_hdr* ip_hdr;
    for (uint32_t i = 0; i < kMaxReqLabel; i++) {
        req_labels.push(i);
        req_label_reply_count[i] = 0;
    }

    while (true) {
        pkt_recv_num = rte_eth_rx_burst(g_nic_info_ctrl_plane.id, queue_config->queue_id, mbufs_recv, kRecvBurstSize);
        for (uint32_t i = 0; i < pkt_recv_num; i++) {
            ip_hdr = rte_pktmbuf_mtod_offset(mbufs_recv[i], struct rte_ipv4_hdr*, sizeof(struct rte_ether_hdr));
            src_ip = ntohl(ip_hdr->src_addr);
            auto iter = std::find(g_lb_ips.begin(), g_lb_ips.end(), src_ip);
            if (iter != g_lb_ips.end())
                f_lb_reply_manage(mbufs_recv[i], queue_config);
            else
                f_virt_info_req_manage(mbufs_recv[i], queue_config);
        }
        for (uint32_t i = 0; i < pkt_recv_num; i++)
            rte_pktmbuf_free(mbufs_recv[i]);
    }
    return 0;
}

void
f_virt_info_req_manage(struct rte_mbuf* recv_req, struct QueueConfig* queue_config)
{
    struct PackedVirtInfoReq* req;
    struct rte_ipv4_hdr* ip_hdr;
    struct rte_udp_hdr* udp_hdr;
    uint32_t hdr_offset = sizeof(struct rte_ether_hdr);
    uint8_t request_type = 2;
    ip_hdr = rte_pktmbuf_mtod_offset(recv_req, struct rte_ipv4_hdr*, hdr_offset);
    hdr_offset += sizeof(struct rte_ipv4_hdr);
    udp_hdr = rte_pktmbuf_mtod_offset(recv_req, struct rte_udp_hdr*, hdr_offset);
    hdr_offset += sizeof(struct rte_udp_hdr);
    req = rte_pktmbuf_mtod_offset(recv_req, struct PackedVirtInfoReq*, hdr_offset);
    struct PackedVirtInfo virt_info = f_virt_info_generate();
    struct PackedVirtInfoKey virt_info_key = {virt_info.virt_ip, virt_info.virt_port, req->protocol};
    PackedEndpoint endpoint = {req->ip, req->port};
    char buffer[sizeof(PackedVirtInfoKey) + sizeof(PackedEndpoint) + 5];
    uint32_t req_label = req_labels.front();
    req_labels.pop();
    req_label_src_info[req_label].ip = ntohl(ip_hdr->src_addr);
    req_label_src_info[req_label].port = ntohs(udp_hdr->src_port);
    req_label_virt_info[req_label] = virt_info;
    req_label_reply_count[req_label] = 0;
    uint32_t buffer_offset = 0;
    memcpy(buffer, &request_type, 1);
    buffer_offset += 1;
    memcpy(buffer + buffer_offset, &virt_info_key, sizeof(PackedVirtInfoKey));
    buffer_offset += sizeof(PackedVirtInfoKey);
    memcpy(buffer + buffer_offset, &endpoint, sizeof(PackedEndpoint));
    buffer_offset += sizeof(PackedEndpoint);
    memcpy(buffer + buffer_offset, &req_label, 4);
    uint32_t pkt_send_num = g_lb_ips.size();
    struct rte_mbuf* mbufs_send[pkt_send_num];
    int status = rte_pktmbuf_alloc_bulk(queue_config->mempool_ptr, mbufs_send, pkt_send_num);
    if (status != 0)
        rte_exit(EXIT_FAILURE, "Error when rte_pktmbuf_alloc_bulk\n");
    struct PackedEndpoint slb_endpoint = {0, 12345};
    for (uint32_t i = 0; i < pkt_send_num; i++) {
        slb_endpoint.ip = g_lb_ips[i];
        f_pkt_construct(mbufs_send[i], g_mac_table[g_lb_ips[i]], buffer, sizeof(buffer), slb_endpoint);
    }
    rte_eth_tx_burst(g_nic_info_ctrl_plane.id, queue_config->queue_id, mbufs_send, pkt_send_num);
}

void
f_lb_reply_manage(struct rte_mbuf* recv_reply, struct QueueConfig* queue_config)
{
    uint32_t* req_label_ptr;
    uint32_t hdr_offset = sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr);
    req_label_ptr = rte_pktmbuf_mtod_offset(recv_reply, uint32_t*, hdr_offset);
    uint32_t req_label = ntohl(*req_label_ptr);
    req_label_reply_count[req_label]++;
    struct rte_mbuf* mbuf_send;
    if (req_label_reply_count[req_label] == g_lb_ips.size()) {
        struct PackedVirtInfo virt_info_reply = req_label_virt_info[req_label];
        struct PackedEndpoint endpoint = req_label_src_info[req_label];
        mbuf_send = rte_pktmbuf_alloc(queue_config->mempool_ptr);
        if (mbuf_send == NULL)
            rte_exit(EXIT_FAILURE, "rte_pktmbuf_alloc failed\n");
        f_pkt_construct(mbuf_send, g_mac_table[endpoint.ip], &virt_info_reply, sizeof(struct PackedVirtInfo), endpoint);
        rte_eth_tx_burst(g_nic_info_ctrl_plane.id, queue_config->queue_id, &mbuf_send, 1);
        req_label_reply_count[req_label] = 0;
        req_label_src_info[req_label].ip = 0;
        req_label_src_info[req_label].port = 0;
        req_label_virt_info[req_label].virt_ip = 0;
        req_label_virt_info[req_label].virt_port = 0;
        req_labels.push(req_label);
    }
}