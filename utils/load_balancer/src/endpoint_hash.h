#ifndef ENDPOINT_HASH_H_
#define ENDPOINT_HASH_H_

#include <unordered_map>
#include <shared_mutex>
#include <mutex>
#include <cstdint>
#include <vector>

struct __attribute__((packed)) PackedThreeTupleKey {
  uint32_t ip;
  uint16_t port;
  uint8_t protocol;
  
  PackedThreeTupleKey() : ip(0), protocol(0), port(0) {}
  PackedThreeTupleKey(uint32_t ip, uint16_t port, uint8_t protocol)
      : ip(ip), port(port), protocol(protocol) {}

  bool operator==(const PackedThreeTupleKey& other) const {
    return ip == other.ip && port == other.port && protocol == other.protocol;
  }
};


struct __attribute__((packed)) PackedFiveTupleKey {
  uint32_t src_ip;
  uint32_t dst_ip;
  uint8_t protocol;
  uint16_t src_port;
  uint16_t dst_port;

  PackedFiveTupleKey() 
      : src_ip(0), dst_ip(0), protocol(0), src_port(0), dst_port(0) {}

  PackedFiveTupleKey(uint32_t src_ip, uint32_t dst_ip, uint8_t protocol, 
                     uint16_t src_port, uint16_t dst_port)
      : src_ip(src_ip), dst_ip(dst_ip), protocol(protocol), 
        src_port(src_port), dst_port(dst_port) {}

  bool operator==(const PackedFiveTupleKey& other) const {
    return src_ip == other.src_ip && src_port == other.src_port &&
           protocol == other.protocol && dst_ip == other.dst_ip &&
           dst_port == other.dst_port;
  }

  PackedFiveTupleKey& operator=(const PackedFiveTupleKey& other) {
    if (this == &other)
      return *this;
    src_ip = other.src_ip;
    dst_ip = other.dst_ip;
    protocol = other.protocol;
    src_port = other.src_port;
    dst_port = other.dst_port;
    return *this; 
  }
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


struct ThreeTupleHash {
  std::size_t operator()(const PackedThreeTupleKey& key) const {
    std::size_t res = key.ip;
    res ^= std::hash<uint8_t>()(key.protocol) + 
           0x9e3779b9 + (res << 6) + (res >> 2);
    res ^= std::hash<uint16_t>()(key.port) + 
           0x9e3779b9 + (res << 6) + (res >> 2);
    return res;
  }
};


struct FiveTupleHash {
  std::size_t operator()(const PackedFiveTupleKey& key) const {
    std::size_t res = key.src_ip;
    res ^= std::hash<uint8_t>()(key.protocol) + 
           0x9e3779b9 + (res << 6) + (res >> 2);
    res ^= key.dst_ip + 0x9e3779b9 + (res << 6) + (res >> 2);
    res ^= std::hash<uint16_t>()(key.src_port) + 
           0x9e3779b9 + (res << 6) + (res >> 2);
    res ^= std::hash<uint16_t>()(key.dst_port) + 
           0x9e3779b9 + (res << 6) + (res >> 2);
    return res;
  }
};


using ThreeTupleToEndpointsMap = std::unordered_map<
    PackedThreeTupleKey, std::vector<PackedEndpoint>, ThreeTupleHash>;

using FiveTupleToEndpointMap = std::unordered_map<
    PackedFiveTupleKey, PackedEndpoint, FiveTupleHash>;

extern std::shared_mutex g_lock_three_tuple_endpoints;
extern std::vector<FiveTupleToEndpointMap> g_map_five_tuple_endpoint;
extern ThreeTupleToEndpointsMap g_map_three_tuple_endpoints;

struct PackedEndpoint f_endpoint_hash(
    FiveTupleToEndpointMap& five_tuple_map,
    const PackedFiveTupleKey& five_tuple_key);
#endif // ENDPOINT_HASH_H_
