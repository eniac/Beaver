#ifndef SS_CONTROL_H_
#define SS_CONTROL_H_

#include <condition_variable>
#include <fstream>
#include <mutex>

#include <rte_arp.h>

#include "virt_info_manage.h"

int
f_ss_pkts_send(__attribute__((unused)) void* arg);

int
f_ss_pkts_recv(__attribute__((unused)) void* arg);

void
f_ss_new_tail_wait(uint32_t ssid, uint32_t max_flight_ss_num);

void
f_arp_mac_req_send(struct QueueConfig* queue_config, uint32_t dst_ip);

void
f_arp_mac_reply_send(struct QueueConfig* queue_config, const std::vector<uint8_t>& dst_mac, uint32_t dst_ip);

int
f_ss_tail_pkts_recv(__attribute__((unused)) void* arg);

uint64_t
f_clock_time_get();

#endif // SS_CONTROL_H_