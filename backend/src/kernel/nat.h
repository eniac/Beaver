#ifndef NAT_H_
#define NAT_H_

#include <bpf/bpf_endian.h>
#include <bpf/bpf_helpers.h>
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/ptrace.h>
#include <linux/tcp.h>
#include <linux/udp.h>

// Define fixed controller ip: 192.168.255.1, network byte order: 0x0101A7C0
const __u32 kControllerIp = 0xC0A8FF01;
const __u8 kProtoUdp = 17;
const __u8 kProtoTcp = 6;
const __u16 kIntraServicePort = 7000;
// Define VIP prefix: 192.168.100.0
const __u32 kVirtIpPrefix = 0xC0A86400;
// Define VIP mask: 255.255.255.0
const __u32 kVirtIpMask = 0xFFFFFF00;

struct __attribute__((packed)) PackedPortProto
{
    __u16 port;
    __u8 protocol;
};

struct __attribute__((packed)) PackedVirtInfo
{
    __u32 virt_ip;
    __u16 virt_port;
};

struct __attribute__((packed)) PackedVirtInfoReq
{
    __u32 ip;
    __u16 port;
    __u8 protocol;
    __u32 client_ip;
    __u16 client_port;
};

/*
 * @brief: When port has been changed, we can use this function to
 *         renew the checksum, this function also applies to 2 bytes update.
 *         (equation: new_checksum = old_checksum + old_port + ~new_port)
 */
static inline __u16
f_csum_update_for_port(__u16 old_port, __u16 new_port, __u16 old_checksum)
{
    __u32 new_checksum = 0;
    __u32 temp = old_checksum + old_port;
    new_checksum = (temp & 0xFFFF) + (temp >> 16);
    // Calculate the complement of new port
    __u16 complement_data = ~new_port & 0xFFFF;
    new_checksum = ((new_checksum + complement_data) & 0xFFFF) + ((new_checksum + complement_data) >> 16);
    return new_checksum & 0xFFFF;
}

/*
 *   @brief: When Ip has been changed, we can use this function to
 *           renew the checksum.
 */
static inline __u16
f_csum_update_for_ip(__u32 old_ip, __u32 new_ip, __u16 old_checksum)
{
    __u16 old_data, new_data;
    old_data = (old_ip) & 0xFFFF;
    new_data = (new_ip) & 0xFFFF;
    __u16 new_checksum = f_csum_update_for_port(old_data, new_data, old_checksum);
    old_data = (old_ip >> 16) & 0xFFFF;
    new_data = (new_ip >> 16) & 0xFFFF;
    new_checksum = f_csum_update_for_port(old_data, new_data, new_checksum);
    return new_checksum;
}
#endif // NAT_H_