#include "initial_data_get.h"


std::vector<uint32_t> f_lb_config_load(const std::string &file_path) {
  std::ifstream config_file(file_path);
  Json::Value json_data;
  config_file >> json_data;
  std::vector<uint32_t> lb_ips;
  for (const auto &node_index : json_data["nodes"]["load_balancer"]) {
    std::string ip_str = "192.168.255." + std::to_string(node_index.asInt());
    uint32_t ip = ntohl(inet_addr(ip_str.c_str()));
    lb_ips.push_back(ip);
  }
  return lb_ips;
}


std::vector<uint32_t> f_backend_config_load(const std::string &file_path) {
    std::ifstream config_file(file_path);
    Json::Value json_data;
    config_file >> json_data;
    std::vector<uint32_t> backend_ips;
    for (const auto &node_index : json_data["nodes"]["backend"]) {
        std::string ip_str = "192.168.255." + std::to_string(node_index.asInt());
        uint32_t ip = ntohl(inet_addr(ip_str.c_str()));
        backend_ips.push_back(ip);
    }
    return backend_ips;
}