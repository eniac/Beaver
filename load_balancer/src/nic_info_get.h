#ifndef NIC_INFO_GET_H_
#define NIC_INFO_GET_H_

#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <string>
#include <vector>

#include <rte_ethdev.h>

struct DpdkNicInfo
{
    std::string name;
    std::vector<uint8_t> mac;
    uint32_t ip;
    uint8_t id;
};

std::vector<uint8_t>
f_nic_mac_get(const std::string& nic_name);

uint8_t
f_nic_id_get(const std::vector<uint8_t>& mac);

std::vector<DpdkNicInfo>
f_nic_info_get(const std::string& ip_prefix);

std::vector<DpdkNicInfo>
f_nic_info_get_data_plane(const std::string& ip_prefix);

#endif // NIC_INFO_GET_H_
