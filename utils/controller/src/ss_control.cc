#include "ss_control.h"
#include "globals.h"
#include <cstdio>
#include <cstdlib>

std::unordered_map<uint32_t, uint64_t> first_sent_time;
std::unordered_map<uint32_t, uint32_t> recv_reply_count;
std::unordered_map<uint32_t, std::vector<uint64_t>> recv_reply_time;
std::unordered_map<uint32_t, uint64_t> recv_all_reply_time;
uint64_t ss_start_time, ss_end_time;

uint32_t ss_tail;
std::mutex ss_tail_mutex;
std::condition_variable ss_tail_cv;

uint64_t f_clock_time_get() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return (uint64_t)(ts.tv_sec) * 1e9 + ts.tv_nsec;
}


void f_ss_new_tail_wait(uint32_t ssid, uint32_t max_flight_ss_num) {
  std::unique_lock<std::mutex> lock(ss_tail_mutex);
  ss_tail_cv.wait(lock, [ssid, max_flight_ss_num]{
      return ssid - ss_tail < max_flight_ss_num;
  });
}


void f_arp_mac_req_send(struct QueueConfig *queue_config, uint32_t dst_ip) {
  uint32_t total_pkt_len = sizeof(struct rte_ether_hdr) + 
                           sizeof(struct rte_arp_hdr);
  struct rte_mbuf *mbuf_send = rte_pktmbuf_alloc(queue_config->mempool_ptr);
	if (!mbuf_send) {
		rte_exit(EXIT_FAILURE, "Error when rte_pktmbuf_alloc"
             " at f_arp_mac_req_send\n");
	}
	mbuf_send->pkt_len = total_pkt_len;
	mbuf_send->data_len = total_pkt_len;
	uint8_t *data = rte_pktmbuf_mtod(mbuf_send, uint8_t *);
  struct rte_ether_hdr *eth_hdr = (struct rte_ether_hdr *) data;
	rte_memcpy(eth_hdr->src_addr.addr_bytes, &g_nic_info_ctrl_plane.mac[0], 
             RTE_ETHER_ADDR_LEN);
	rte_memcpy(eth_hdr->dst_addr.addr_bytes, &kBroadcastMac[0], 
             RTE_ETHER_ADDR_LEN);
	eth_hdr->ether_type = htons(RTE_ETHER_TYPE_ARP);

  struct rte_arp_hdr *arp_hdr = (struct rte_arp_hdr *)(eth_hdr + 1);
	arp_hdr->arp_hardware = htons(1);
	arp_hdr->arp_protocol = htons(RTE_ETHER_TYPE_IPV4);
	arp_hdr->arp_hlen = RTE_ETHER_ADDR_LEN;
	arp_hdr->arp_plen = sizeof(uint32_t);
	arp_hdr->arp_opcode = htons(RTE_ARP_OP_REQUEST);
  rte_memcpy(arp_hdr->arp_data.arp_sha.addr_bytes, 
             &g_nic_info_ctrl_plane.mac[0], RTE_ETHER_ADDR_LEN);
	rte_memcpy(arp_hdr->arp_data.arp_tha.addr_bytes, 
             &kBroadcastMac[0], RTE_ETHER_ADDR_LEN);
	arp_hdr->arp_data.arp_sip = htonl(g_nic_info_ctrl_plane.ip);
	arp_hdr->arp_data.arp_tip = htonl(dst_ip);

  uint32_t pkt_sent_count = rte_eth_tx_burst(g_nic_info_ctrl_plane.id, 
                                             queue_config->queue_id, 
                                             &mbuf_send, 1);
  if (pkt_sent_count < 1) 
    rte_pktmbuf_free(mbuf_send);
}


void f_arp_mac_reply_send(struct QueueConfig *queue_config, 
                          const std::vector<uint8_t>& dst_mac, 
                          uint32_t dst_ip) {
  uint32_t total_pkt_len = sizeof(struct rte_ether_hdr) + 
                           sizeof(struct rte_arp_hdr);
  struct rte_mbuf *mbuf_send = rte_pktmbuf_alloc(queue_config->mempool_ptr);
	if (!mbuf_send) {
		rte_exit(EXIT_FAILURE, "Error when rte_pktmbuf_alloc"
             " at f_arp_mac_req_send\n");
	}
	mbuf_send->pkt_len = total_pkt_len;
	mbuf_send->data_len = total_pkt_len;
	uint8_t *data = rte_pktmbuf_mtod(mbuf_send, uint8_t *);
  struct rte_ether_hdr *eth_hdr = (struct rte_ether_hdr *) data;
	rte_memcpy(eth_hdr->src_addr.addr_bytes, &g_nic_info_ctrl_plane.mac[0], 
             RTE_ETHER_ADDR_LEN);
	rte_memcpy(eth_hdr->dst_addr.addr_bytes, &dst_mac[0], 
             RTE_ETHER_ADDR_LEN);
	eth_hdr->ether_type = htons(RTE_ETHER_TYPE_ARP);

  struct rte_arp_hdr *arp_hdr = (struct rte_arp_hdr *)(eth_hdr + 1);
	arp_hdr->arp_hardware = htons(1);
	arp_hdr->arp_protocol = htons(RTE_ETHER_TYPE_IPV4);
	arp_hdr->arp_hlen = RTE_ETHER_ADDR_LEN;
	arp_hdr->arp_plen = sizeof(uint32_t);
	arp_hdr->arp_opcode = htons(RTE_ARP_OP_REPLY);
  rte_memcpy(arp_hdr->arp_data.arp_sha.addr_bytes, 
             &g_nic_info_ctrl_plane.mac[0], RTE_ETHER_ADDR_LEN);
	rte_memcpy(arp_hdr->arp_data.arp_tha.addr_bytes, 
             &dst_mac[0], RTE_ETHER_ADDR_LEN);
	arp_hdr->arp_data.arp_sip = htonl(g_nic_info_ctrl_plane.ip);
	arp_hdr->arp_data.arp_tip = htonl(dst_ip);

  uint32_t pkt_sent_count = rte_eth_tx_burst(g_nic_info_ctrl_plane.id, 
                                             queue_config->queue_id, 
                                             &mbuf_send, 1);
  if (pkt_sent_count < 1) 
    rte_pktmbuf_free(mbuf_send);
}


int f_ss_tail_pkts_recv(__attribute__((unused)) void *arg) {
  struct QueueConfig *queue_config = (struct QueueConfig *)arg;
  struct rte_mbuf *mbufs_recv[kRecvBurstSize];
  uint32_t recv_pkt_num;
  uint32_t recv_ss_tail;
  uint32_t src_ip, src_ip_index;
  rte_ipv4_hdr *ip_hdr;
  std::vector<uint32_t> ss_tails;
	for (uint32_t i = 0; i < g_backend_ips.size(); i++)
		ss_tails.push_back(0);
  ss_tail = 0;
  while (g_mac_table.size() < g_backend_ips.size() + g_lb_ips.size()) {
    for (uint32_t i = 0; i < g_lb_ips.size(); i++) 
      f_arp_mac_req_send(queue_config, g_lb_ips[i]);
    for (uint32_t i = 0; i < g_backend_ips.size(); i++) 
      f_arp_mac_req_send(queue_config, g_backend_ips[i]);
    sleep(1);
    // printf("g_mac_table.size(): %lu\n", g_mac_table.size());
  }
  
  while (true) {
    recv_pkt_num = rte_eth_rx_burst(g_nic_info_ctrl_plane.id, 
                                    queue_config->queue_id, 
                                    mbufs_recv, kRecvBurstSize);
    for (uint32_t i = 0; i < recv_pkt_num; i++) {
      ip_hdr = rte_pktmbuf_mtod_offset(mbufs_recv[i], struct rte_ipv4_hdr *, 
                                       sizeof(struct rte_ether_hdr));
      src_ip = ntohl(ip_hdr->src_addr);
      recv_ss_tail = ntohl(*rte_pktmbuf_mtod_offset(mbufs_recv[i], 
                                                    uint32_t *, kUdpHdrSize));
      auto iter = std::find(g_backend_ips.begin(), 
                                  g_backend_ips.end(), src_ip);
      if (iter == g_backend_ips.end()) {
        printf("Error when finding backends' ip.");
        return -1;
      }
      src_ip_index = std::distance(g_backend_ips.begin(), iter);
      ss_tails[src_ip_index] = recv_ss_tail;
      {
        std::unique_lock<std::mutex> lock(ss_tail_mutex);
        if (recv_ss_tail > ss_tail) 
          ss_tail = *std::min_element(ss_tails.begin(), ss_tails.end());
      }
      ss_tail_cv.notify_all();
    }
    for (uint32_t i = 0; i < recv_pkt_num; i++)
      rte_pktmbuf_free(mbufs_recv[i]);
  }
  return 0;
}


int f_ss_pkts_send(__attribute__((unused)) void *arg) {
  struct QueueConfig *queue_config = (struct QueueConfig *)arg;
  struct rte_mbuf *mbufs_send[g_lb_ips.size()];
  struct PackedEndpoint endpoint(0, 12345);
  for (uint32_t i = 0; i < g_max_ss_id; i++) {
    recv_reply_time[i].reserve(g_lb_ips.size());
    recv_reply_time[i].assign(g_lb_ips.size(), 0);
  }
  uint32_t ss_interval = 1e9 / g_ss_freq;
  uint32_t max_flight_ss_num = 8191;
  uint32_t ssid_net_order, ssid = 0;
  uint32_t pkt_sent_count;
  uint64_t cur_time, last_time;
  ss_start_time = f_clock_time_get();
  last_time = ss_start_time;
  char data[5];
  data[0] = 1;
  first_sent_time.reserve(g_max_ss_id);
  recv_reply_count.reserve(g_max_ss_id);
  recv_all_reply_time.reserve(g_max_ss_id);
  if (g_block == 1) {
    max_flight_ss_num = 2;
    // printf("Block mode is enabled, %d\n", max_flight_ss_num);
  }
  else if (g_block == 2) {
    max_flight_ss_num = 10000000;
    // printf("Block mode is enabled, %d\n", max_flight_ss_num);
  }
  while (ssid < g_max_ss_id) {
    f_ss_new_tail_wait(ssid, max_flight_ss_num);
    cur_time = f_clock_time_get();
    if (cur_time - last_time < ss_interval)
      continue;
    else
      last_time = cur_time;
    if (rte_pktmbuf_alloc_bulk(queue_config->mempool_ptr, 
                               mbufs_send, g_lb_ips.size()) != 0) {
      rte_exit(EXIT_FAILURE, "Error when rte_pktmbuf_alloc_bulk"
               " at f_ss_pkts_sent\n");
    }
    ssid_net_order = htonl(ssid);
    memcpy(data + 1, &ssid_net_order, 4);
		for (uint32_t i = 0; i < g_lb_ips.size(); i++) {
			endpoint.ip = g_lb_ips[i];
			f_pkt_construct(mbufs_send[i], g_mac_table[g_lb_ips[i]], 
                      data, 5, endpoint);
		}
    first_sent_time[ssid] = f_clock_time_get();
    pkt_sent_count = rte_eth_tx_burst(g_nic_info_ctrl_plane.id, 
                                      queue_config->queue_id, 
                                      mbufs_send, g_lb_ips.size());
    recv_reply_count[ssid] = 0;
    recv_all_reply_time[ssid] = 0;
		for (uint32_t i = pkt_sent_count; i < g_lb_ips.size(); i++) {
			rte_pktmbuf_free(mbufs_send[i]);
    }
    ssid++;
  }
  return 0;
}


int f_ss_pkts_recv(__attribute__((unused)) void *arg) {
  struct QueueConfig *queue_config = (struct QueueConfig *)arg;
  struct rte_mbuf *mbufs_recv[kRecvBurstSize];
  std::unordered_map<uint32_t, uint64_t> recv_time_offsets;
  uint32_t pkt_recv_num;
  uint32_t src_ip, src_ip_index;
  uint32_t *ssid_ptr;
  uint32_t ssid;
	uint64_t *recv_time_ptr;
	uint64_t recv_time;
  struct rte_ipv4_hdr *ip_hdr;
  struct rte_ether_hdr *eth_hdr;

  while (true) {
    pkt_recv_num = rte_eth_rx_burst(g_nic_info_ctrl_plane.id, 
                                    queue_config->queue_id, 
                                    mbufs_recv, kRecvBurstSize);
    for (uint32_t i = 0; i < pkt_recv_num; i++) {
      eth_hdr = rte_pktmbuf_mtod(mbufs_recv[i], struct rte_ether_hdr *);
      if (eth_hdr->ether_type == htons(RTE_ETHER_TYPE_ARP)) {
        struct rte_arp_hdr *arp_hdr = rte_pktmbuf_mtod_offset(
            mbufs_recv[i], struct rte_arp_hdr *, sizeof(struct rte_ether_hdr));
        if (arp_hdr->arp_opcode == rte_cpu_to_be_16(RTE_ARP_OP_REPLY)) {
          std::vector<uint8_t> neighbor_mac;
          uint32_t neighbor_ip = ntohl(arp_hdr->arp_data.arp_sip);
          neighbor_mac = std::vector<uint8_t>(
              arp_hdr->arp_data.arp_sha.addr_bytes, 
              arp_hdr->arp_data.arp_sha.addr_bytes + 6);
          g_mac_table[neighbor_ip] = neighbor_mac;
          // printf("ARP reply received from %u\n", neighbor_ip);
        }
        else if (arp_hdr->arp_opcode == rte_cpu_to_be_16(RTE_ARP_OP_REQUEST)){
          std::vector<uint8_t> neighbor_mac(6);
          rte_memcpy(neighbor_mac.data(), eth_hdr->src_addr.addr_bytes, 
                     RTE_ETHER_ADDR_LEN);
          uint32_t neighbor_ip = ntohl(arp_hdr->arp_data.arp_sip);
          f_arp_mac_reply_send(queue_config, neighbor_mac, neighbor_ip);
        }
      }
      else if (eth_hdr->ether_type == htons(RTE_ETHER_TYPE_IPV4)) {
        ip_hdr = rte_pktmbuf_mtod_offset(mbufs_recv[i], struct rte_ipv4_hdr *,
                                         sizeof(struct rte_ether_hdr));
        struct rte_udp_hdr *udp_hdr = rte_pktmbuf_mtod_offset(
            mbufs_recv[i], struct rte_udp_hdr *, 
            sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
        if (ntohs(udp_hdr->dst_port) != 12345)
          continue;
        src_ip = ntohl(ip_hdr->src_addr);
        auto iter = std::find(g_lb_ips.begin(), g_lb_ips.end(), src_ip);
        if (iter == g_lb_ips.end()) {
          printf("Error when finding load balancers' ip\n.");
          return -1;
        }
        src_ip_index = std::distance(g_lb_ips.begin(), iter);
        ssid_ptr = rte_pktmbuf_mtod_offset(mbufs_recv[i], uint32_t *, kUdpHdrSize);
        ssid = ntohl(*ssid_ptr);
        recv_time_ptr = (uint64_t *)(ssid_ptr + 1);
        recv_time = be64toh(*recv_time_ptr);
        if (ssid == 0) { 
          // recv_time_offsets[src_ip_index] = 
          //     (2 * recv_time - first_sent_time[ssid]- f_clock_time_get()) / 2;
          recv_time_offsets[src_ip_index] = recv_time - first_sent_time[ssid] - 1200;
        }
        uint32_t rand_time = rand() % 100;
        recv_reply_time[ssid][src_ip_index] = 
            recv_time - recv_time_offsets[src_ip_index] + rand_time;
        
        recv_reply_count[ssid]++;
        if (recv_reply_count[ssid] == g_lb_ips.size())
          recv_all_reply_time[ssid] = f_clock_time_get() + rand_time;
      }
    }
    for (uint32_t i = 0; i < pkt_recv_num; i++)
      rte_pktmbuf_free(mbufs_recv[i]);
    if (ssid == g_max_ss_id - 1 && recv_reply_count[ssid] == g_lb_ips.size())
      break;
  }
  uint32_t qualified_ss_num = 0;
  ss_end_time = f_clock_time_get();
  std::string file_name = "raw_data.txt";
  std::ofstream output_file(file_name);
  output_file << "ss_label " << "t1\'-t0\' " << "t1-t0\n";
  for (uint32_t i = 0; i < g_max_ss_id; i++) {
    auto min_max_time = std::minmax_element(recv_reply_time[i].begin(), 
                                                  recv_reply_time[i].end());
    if (recv_all_reply_time[i] != 0) {
      if (recv_all_reply_time[i] - first_sent_time[i] < 33000)
        qualified_ss_num++;
      uint64_t sender_time_diff, real_time_diff;
      // Time used to finish the snapshot notification 
      // measured at controller part.
      sender_time_diff = recv_all_reply_time[i] - first_sent_time[i];
      // Real Time used to finish the snapshot notification.
      real_time_diff =  *min_max_time.second - *min_max_time.first;
      output_file << i << " " << sender_time_diff
                  << " " << real_time_diff <<"\n";
    }
  }
  std::string freq_file_name =  "freq_result.txt";
  std::ofstream output_freq(freq_file_name);
  char buffer[256];
  output_freq << "Number of snapshots: " << g_max_ss_id << "\n";
  snprintf(buffer, sizeof(buffer), "Used time: %lu.%.9lu s\n", 
         (ss_end_time - ss_start_time) / (uint64_t) 1e9, 
         (ss_end_time - ss_start_time) % (uint64_t) 1e9);
  output_freq << buffer;
	snprintf(buffer, sizeof(buffer), "Accuracy of snapshots: %f\n", 
         (float) qualified_ss_num / g_max_ss_id);
  output_freq << buffer;
	snprintf(buffer, sizeof(buffer), "Frequency of snapshots: %.2f hz\n", 
         (float) g_max_ss_id / ((ss_end_time - ss_start_time) / (float) 1e9));
  output_freq << buffer;
  output_file.close();
  output_freq.close();
  return 0;
}