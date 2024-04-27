#ifndef LB_REDIRECT_H_
#define LB_REDIRECT_H_

#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/in.h>
#include <linux/ptrace.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>


const __u32 kArraySize = 1024;
const __u32 kHashTableSize = 4096;
// The port larger than kDstPortRedirect will be redirected to load balancers
const __u32 kDstPortRedirect = 30000;


/*
* @brief: When port has been changed, we can use this function to 
*         renew the checksum, this function also applies to 2 bytes update.
*         (equation: new_checksum = old_checksum + old_port + ~new_port)
*/
static inline __u16 f_csum_update_for_port(__u16 old_port, __u16 new_port,
                                           __u16 old_checksum) {
  __u32 new_checksum = 0;
  __u32 temp = old_checksum + old_port;
  new_checksum = (temp & 0xFFFF) + (temp >> 16);
  // Calculate the complement of new port
  __u16 complement_data = ~new_port & 0xFFFF;
  new_checksum = ((new_checksum + complement_data) & 0xFFFF) +
                 ((new_checksum + complement_data) >> 16);
  return new_checksum & 0xFFFF;
}


/*
*   @brief: When Ip has been changed, we can use this function to 
*           renew the checksum. 
*/
static inline __u16 f_csum_update_for_ip(__u32 old_ip, __u32 new_ip, 
                                         __u16 old_checksum) {
  __u16 old_data, new_data;
  old_data = (old_ip) & 0xFFFF;
  new_data = (new_ip) & 0xFFFF;
  __u16 new_checksum = f_csum_update_for_port(old_data, new_data, old_checksum);
  old_data = (old_ip >> 16) & 0xFFFF;
  new_data = (new_ip >> 16) & 0xFFFF;
  new_checksum = f_csum_update_for_port(old_data, new_data, new_checksum);
  return new_checksum;
}


/*
*  @brief: Get the index of a load balancer by hashing
*/
static inline __u32 f_lb_index_hash(__u32 src_ip, __u32 dst_ip, 
                                    __u16 src_port, __u16 dst_port, 
                                    __u8 protocol, __u16 lb_number) {
  __u32 hash_value = src_ip ^ dst_ip;
  hash_value ^= ((__u32)src_port << 16);
  hash_value ^= ((__u32)dst_port);
  hash_value ^= protocol;
  __u32 lb_index = hash_value % lb_number;
  return lb_index;
}


static inline __u32 f_port_hash_key_get(__u16 src_port, __u16 dst_port) {
  __u32 hash_value = ((__u32)src_port << 16) ^ ((__u32)dst_port);
  return hash_value;
}

#endif // LB_REDIRECT_H_