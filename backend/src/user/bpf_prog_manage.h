#ifndef BPF_PROGRAM_MANAGE_H_
#define BPF_PROGRAM_MANAGE_H_

#include <cstdio>
#include <cstring>
#include <unistd.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <linux/bpf.h>
#include <linux/if_link.h>

const int kSyscallBpfX86 = 321;

// Command for bpf syscall
static int
f_bpf_syscall(enum bpf_cmd cmd, union bpf_attr* attr, unsigned int size)
{
    return syscall(kSyscallBpfX86, cmd, attr, size);
}

int
f_bpf_obj_get(const char* pathname);

int
f_bpf_egress_prog_load(const char* prog_pathname, int if_index);

int
f_bpf_ingress_prog_load(const char* prog_pathname, int if_index);

void
initial_bot();

int
f_bpf_prog_unload(int if_index);

int
f_nic_ifindex_get_data_plane(const char* ip_prefix);

#endif // BPF_PROGRAM_MANAGE_H_