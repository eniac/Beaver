#ifndef BPF_VIRT_INFO_REQ_H_
#define BPF_VIRT_INFO_REQ_H_

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <thread>

#include <netdb.h>
#include <stdint.h>

#include "bpf_prog_manage.h"

static const char* kControllerIp = "192.168.255.1";

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

void
f_virt_info_req(__u16 port);

void
f_ss_tail_send(uint16_t src_port, uint16_t dst_port);

#endif // BPF_VIRT_INFO_REQ_H_