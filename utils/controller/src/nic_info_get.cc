#include "nic_info_get.h"


std::vector<uint8_t> f_nic_mac_get(const std::string& nic_name) {
  struct ifreq if_request;
  int sock_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
  if_request.ifr_addr.sa_family = AF_INET;
  strncpy(if_request.ifr_name, nic_name.c_str(), IFNAMSIZ - 1);
  ioctl(sock_descriptor, SIOCGIFHWADDR, &if_request);
  close(sock_descriptor);
  
  std::vector<uint8_t> mac_address(6);
  for (int i = 0; i < 6; ++i)
    mac_address[i] = (unsigned char)if_request.ifr_hwaddr.sa_data[i];
  return mac_address;
}


uint8_t f_nic_id_get(const std::vector<uint8_t>& mac) {
  uint8_t nic_id;
  struct rte_ether_addr eth_addr;
  for (nic_id = 0; nic_id < RTE_MAX_ETHPORTS; nic_id++) {
    if (!rte_eth_dev_is_valid_port(nic_id))
      continue;
    rte_eth_macaddr_get(nic_id, &eth_addr);
    bool is_match = true;
    for (int i = 0; i < 6; ++i) {
      if (eth_addr.addr_bytes[i] != mac[i]) {
        is_match = false;
        break;
      }
    }
    if (is_match)
      return nic_id;
  }
  return RTE_MAX_ETHPORTS;
}


std::vector<DpdkNicInfo> f_nic_info_get(const std::string& ip_prefix) {
  std::vector<DpdkNicInfo> nics_info;
  struct ifaddrs *iface_addrs, *curr_iface;
  int addr_family, status;
  char host_addr[NI_MAXHOST];

  if (getifaddrs(&iface_addrs) == -1) {
    perror("Error when getting iface address.");
    exit(EXIT_FAILURE);
  }

  for (curr_iface = iface_addrs; curr_iface != NULL; 
       curr_iface = curr_iface->ifa_next) {
    if (curr_iface->ifa_addr == NULL)
      continue;
    addr_family = curr_iface->ifa_addr->sa_family;
    if (addr_family == AF_INET || addr_family == AF_INET6) {
      uint32_t sock_size = (addr_family == AF_INET) 
                           ? sizeof(struct sockaddr_in) 
                           : sizeof(struct sockaddr_in6);
      status = getnameinfo(curr_iface->ifa_addr, sock_size,
                           host_addr, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
      if (status != 0) {
        perror("Error when getting name info.");
        exit(EXIT_FAILURE);
      }
      std::string ip_addr(host_addr);
      if (ip_addr.find(ip_prefix) == 0) {
        DpdkNicInfo nic_info;
        nic_info.name = curr_iface->ifa_name;
        nic_info.ip = ntohl(inet_addr(ip_addr.c_str()));
        nic_info.mac = f_nic_mac_get(nic_info.name);
        nic_info.id = f_nic_id_get(nic_info.mac);
        nics_info.push_back(nic_info);
      }
    }
  }
  freeifaddrs(iface_addrs);
  return nics_info;
}
