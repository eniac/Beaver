#include "dpdk_initial.h"


void f_dpdk_nic_queue_initial(struct rte_mempool* mempool_ptr, 
                              int dpdk_nic_id, uint32_t queue_num) {
  if (rte_eth_dev_count_avail() == 0)
    rte_exit(EXIT_FAILURE, "No Supported NICs found.\n");
  struct rte_eth_dev_info dev_info;
  struct rte_eth_conf nic_conf = {
    .rxmode = {.max_lro_pkt_size = RTE_ETHER_MAX_LEN,},};
  uint64_t offloads = nic_conf.rxmode.offloads;
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

    // Set ARP packets to queue 0
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
    rte_exit(EXIT_FAILURE, "Error creating ARP flow. Message: %s\n", 
             error.message);
  }

  memset(pattern, 0, sizeof(pattern));
  pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;
  pattern[1].type = RTE_FLOW_ITEM_TYPE_IPV4;
  pattern[2].type = RTE_FLOW_ITEM_TYPE_TCP;
  pattern[3].type = RTE_FLOW_ITEM_TYPE_END;

  uint16_t rss_queues[queue_num];
  for (int i = 0; i < queue_num; i++)
    rss_queues[i] = i;

  struct rte_flow_action_rss rss_action = {
      .types = RTE_ETH_RSS_NONFRAG_IPV4_TCP | 
               RTE_ETH_RSS_NONFRAG_IPV4_UDP,
      .key_len = 40,
      .queue_num = queue_num,
      .key = kRssKey,
      .queue = rss_queues,};
  
  memset(action, 0, sizeof(action));
  action[0].type = RTE_FLOW_ACTION_TYPE_RSS;
  action[0].conf = &rss_action;
  action[1].type = RTE_FLOW_ACTION_TYPE_END;
  if (!rte_flow_create(dpdk_nic_id, &attr, pattern, action, &error)) {
    rte_exit(EXIT_FAILURE, "Error creating RSS flow. Message: %s\n", 
             error.message);
  }

  pattern[2].type = RTE_FLOW_ITEM_TYPE_UDP;
  if (!rte_flow_create(dpdk_nic_id, &attr, pattern, action, &error)) {
    rte_exit(EXIT_FAILURE, "Error creating RSS flow. Message: %s\n", 
             error.message);
  }
}
