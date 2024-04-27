#ifndef LOAD_BALANCE_H_
#define LOAD_BALANCE_H_

#include <mutex>
#include <algorithm>
#include <fstream> 

#include <rte_arp.h>
#include <sys/types.h>
#include "dpdk_initial.h"

int f_load_balance(__attribute__((unused)) void *arg);

int f_ctrl_pkts_update(__attribute__((unused)) void *arg);

int f_ctrl_pkt_handle(struct rte_mbuf *recv_pkt, struct rte_mbuf *mbuf_send);

bool f_load_balance_pkt(struct rte_mbuf *recv_pkt, 
                        struct rte_mbuf *mbuf_send, 
                        uint16_t queue_id);

void f_pkt_construct_data_plane(struct rte_mbuf* mbuf, 
                                const std::vector<uint8_t>& neighbor_mac,
                                void *data, uint32_t data_length, 
                                const struct PackedEndpoint &endpoint);
                            
void f_pkt_construct_ctrl_plane(struct rte_mbuf* mbuf, 
                                const std::vector<uint8_t>& neighbor_mac,
                                void *data, uint32_t data_length, 
                                const struct PackedEndpoint &endpoint);

void f_pkt_construct_arp_reply(struct rte_mbuf* mbuf, 
                               const std::vector<uint8_t>& dst_mac,
                               uint32_t dst_ip);

uint64_t f_clock_time_get();

#endif // LOAD_BALANCE_H_
