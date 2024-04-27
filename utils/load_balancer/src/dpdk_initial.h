#ifndef DPDK_INITIAL_H_
#define DPDK_INITIAL_H_

#include <rte_eal.h>
#include <rte_flow.h>
#include <rte_ethdev.h>

#include "globals.h"

struct QueueConfig {
  struct rte_mempool* mempool_ptr;
  uint16_t queue_id;

  QueueConfig(struct rte_mempool* mempool_ptr, uint16_t queue_id)
    : mempool_ptr(mempool_ptr), queue_id(queue_id) {}
};


void f_dpdk_nic_queue_initial(struct rte_mempool* mempool_ptr, 
                              int dpdk_nic_id, uint32_t queue_num);
#endif // DPDK_INITIAL_H_
