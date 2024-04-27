#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <cstdint>
#include <sys/types.h>
#include <unordered_map>

#include "nic_info_get.h"

extern const uint32_t kMbufsNum;
extern const uint32_t kMbufsCacheSize;
extern const uint32_t kRecvBurstSize;
extern const uint32_t kUdpHdrSize;
extern const uint8_t  kRssKey[];
extern const uint8_t  kBroadcastMac[];

// Global constants that need initialization
// based on nodes'configuration
extern struct DpdkNicInfo g_nic_info_ctrl_plane;
extern uint16_t g_local_port_ctrl_plane;
extern uint32_t g_max_ss_id;
extern uint32_t g_ss_freq;
extern uint32_t g_block;

extern std::vector<uint32_t> g_lb_ips;
extern std::vector<uint32_t> g_backend_ips;

extern std::unordered_map<uint32_t, std::vector<uint8_t>> g_mac_table;
#endif // GLOBALS_H_
