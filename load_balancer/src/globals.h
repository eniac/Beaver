#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <cstdint>
#include <vector>

#include "endpoint_hash.h"
#include "nic_info_get.h"

extern const uint32_t kMbufsNum;
extern const uint32_t kMbufsCacheSize;
extern const uint32_t kRecvBurstSize;
extern const uint32_t kUdpHdrSize;
extern const uint8_t kRssKey[];
extern const uint8_t kBroadcastMac[];

// Global constants that need initialization
// based on nodes'configuration
extern struct DpdkNicInfo g_nic_info_data_plane;
extern struct DpdkNicInfo g_nic_info_ctrl_plane;
extern uint32_t g_controller_ip;
extern uint32_t g_virt_ip_prefix;
extern uint32_t g_virt_ip_mask;
extern uint16_t g_local_port_data_plane;
extern uint16_t g_local_port_ctrl_plane;

extern bool g_if_test_latency;
#endif // GLOBALS_H_
