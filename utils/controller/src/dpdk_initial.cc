#include "dpdk_initial.h"


void f_dpdk_nic_queue_initial(struct rte_mempool* mempool_ptr, 
                              int dpdk_nic_id) {
  if (rte_eth_dev_count_avail() == 0)
    rte_exit(EXIT_FAILURE, "No Supported NICs found.\n");
  struct rte_eth_dev_info dev_info;
  struct rte_eth_conf nic_conf = {
    .rxmode = {.max_lro_pkt_size = RTE_ETHER_MAX_LEN,},};
  uint64_t offloads = nic_conf.rxmode.offloads;
  uint32_t queue_num = 3;

  rte_eth_dev_info_get(dpdk_nic_id, &dev_info);
  rte_eth_dev_configure(dpdk_nic_id, queue_num, queue_num, &nic_conf);
  for (int i = 0; i < queue_num; i++) {
    if (rte_eth_rx_queue_setup(dpdk_nic_id, i, 1024, 
                               rte_eth_dev_socket_id(dpdk_nic_id), 
                               NULL, mempool_ptr) < 0) {
      rte_exit(EXIT_FAILURE, "Could not setup RX queue %d.\n", i);
    }
    struct rte_eth_txconf tx_conf = dev_info.default_txconf;
    tx_conf.offloads = offloads;
    if (rte_eth_tx_queue_setup(dpdk_nic_id, i, 1024, 
                               rte_eth_dev_socket_id(dpdk_nic_id), 
                               &tx_conf) < 0) {
      rte_exit(EXIT_FAILURE, "Could not setup TX queue %d.\n", i);
    }
  }

  if (rte_eth_dev_start(dpdk_nic_id) < 0)
    rte_exit(EXIT_FAILURE, "Could not start initialization.\n");

  struct rte_flow_attr attr;
  struct rte_flow_item pattern[4];
  struct rte_flow_action action[2];
  struct rte_flow_error error;
  memset(&attr, 0, sizeof(attr));
  attr.ingress = 1;

  memset(pattern, 0, sizeof(pattern));
  pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;
  struct rte_flow_item_eth eth_spec = {.type = RTE_BE16(RTE_ETHER_TYPE_ARP)};
  pattern[0].spec = &eth_spec;
  struct rte_flow_item_eth eth_mask = {.type = 0xFFFF};
  pattern[0].mask = &eth_mask;
  pattern[1].type = RTE_FLOW_ITEM_TYPE_END;

  memset(action, 0, sizeof(action));
  action[0].type = RTE_FLOW_ACTION_TYPE_QUEUE;
  struct rte_flow_action_queue queue_conf = {.index = 0};
  action[0].conf = &queue_conf;
  action[1].type = RTE_FLOW_ACTION_TYPE_END;
  if (!rte_flow_create(dpdk_nic_id, &attr, pattern, action, &error)) {
    rte_exit(EXIT_FAILURE, "Error initialize arp"
             " at f_dpdk_nic_queue_initial.");
  }

  uint16_t dst_port = 12345;
  struct rte_flow_item_udp udp_spec;
  struct rte_flow_item_udp udp_mask;
  memset(&udp_spec, 0, sizeof(udp_spec));
  memset(&udp_mask, 0, sizeof(udp_mask));
  udp_spec.hdr.dst_port = rte_cpu_to_be_16(dst_port);
  udp_mask.hdr.dst_port = 0xffff;

  memset(pattern, 0, sizeof(pattern));
  pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;
  pattern[1].type = RTE_FLOW_ITEM_TYPE_IPV4;
  pattern[2].type = RTE_FLOW_ITEM_TYPE_UDP;
  pattern[2].spec = &udp_spec;
  pattern[2].mask = &udp_mask;
  pattern[3].type = RTE_FLOW_ITEM_TYPE_END;
  if (!rte_flow_create(dpdk_nic_id, &attr, pattern, action, &error)) {
    rte_exit(EXIT_FAILURE, "Error initialize udp for queue 0"
             " at f_dpdk_nic_queue_initial.");
  }

  dst_port = 60000;
  udp_spec.hdr.dst_port = rte_cpu_to_be_16(dst_port);
  pattern[2].spec = &udp_spec;
  queue_conf.index = 1;
  action[0].conf = &queue_conf;
  if (!rte_flow_create(dpdk_nic_id, &attr, pattern, action, &error)) {
    rte_exit(EXIT_FAILURE, "Error initialize udp for queue 1"
             " at f_dpdk_nic_queue_initial.");
  }

  dst_port = 50000;
  udp_spec.hdr.dst_port = rte_cpu_to_be_16(dst_port);
  pattern[2].spec = &udp_spec;
  queue_conf.index = 2;
  action[0].conf = &queue_conf;
  if (!rte_flow_create(dpdk_nic_id, &attr, pattern, action, &error)) {
    rte_exit(EXIT_FAILURE, "Error initialize udp for queue 2"
             " at f_dpdk_nic_queue_initial.");
  } 
}