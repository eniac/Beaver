#ifndef VIRT_INFO_MANAGE_H_
#define VIRT_INFO_MANAGE_H_

#include <queue>
#include <unordered_map>
#include <algorithm>
#include <set>
#include <ctime> 

#include "dpdk_initial.h"


const uint32_t kMaxReqLabel = 50000;
const uint32_t kMinVirtPort = 30000;

struct __attribute__((packed)) PackedVirtInfoKey {
  uint32_t virt_ip;
  uint16_t virt_port;
  uint8_t  protocol;
};

struct __attribute__((packed)) PackedVirtInfo {
  uint32_t virt_ip;
  uint16_t virt_port;
};

struct __attribute__((packed)) PackedVirtInfoReq {
  uint32_t ip;
  uint16_t port;
  uint8_t  protocol;
  uint32_t client_ip;
  uint16_t client_port;
};

struct __attribute__((packed)) PackedEndpoint {
  uint32_t ip;
  uint16_t port;

  PackedEndpoint() : ip(0), port(0) {}
  PackedEndpoint(uint32_t ip, uint16_t port) : ip(ip), port(port) {}
    
  bool is_zero() const {
    return ip == 0 && port == 0;
  }
  bool operator==(const PackedEndpoint& other) const {
    return this->ip == other.ip && this->port == other.port;
  }
};

int f_virt_info_reqs_manage(__attribute__((unused)) void *arg);

void f_virt_info_req_manage(struct rte_mbuf *recv_req, 
                            struct QueueConfig *queue_config);
                        
void f_lb_reply_manage(struct rte_mbuf *recv_reply,
                       struct QueueConfig *queue_config);

void f_pkt_construct(struct rte_mbuf* mbuf, 
                     const std::vector<uint8_t>& neighbor_mac,
                     void *data, uint32_t data_length, 
                     const struct PackedEndpoint &endpoint);

struct PackedVirtInfo f_virt_info_generate();

#endif // VIRT_INFO_MANAGE_H_
