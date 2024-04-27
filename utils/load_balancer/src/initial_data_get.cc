#include "initial_data_get.h"


ThreeTupleToEndpointsMap f_initial_data_get(const std::string &file_path) {
  std::ifstream initial_file(file_path);
  Json::Value json_data;
  initial_file >> json_data;

  std::unordered_map<std::string, uint8_t> protocol_map = 
      {{"tcp", 6}, {"udp", 17}};
  ThreeTupleToEndpointsMap three_tuple_map;
  for (const auto &virt_info_entry : json_data["services"]) {
    PackedThreeTupleKey three_tuple_key;
    three_tuple_key.ip = 
        ntohl(inet_addr(virt_info_entry["virt_ip"].asCString()));
    auto port_str = virt_info_entry["virt_port"].asString();
    three_tuple_key.port = static_cast<uint16_t>(std::stoi(port_str));
    three_tuple_key.protocol = 
        protocol_map[virt_info_entry["protocol"].asString()];
    
    std::vector<PackedEndpoint> endpoints;
    PackedEndpoint endpoint;
    endpoint.ip = ntohl(inet_addr(virt_info_entry["ip"].asCString()));
    auto port_endpoint_str = virt_info_entry["port"].asString();
    endpoint.port = static_cast<uint16_t>(std::stoi(port_endpoint_str));
    endpoints.push_back(endpoint);
    
    if (three_tuple_map.find(three_tuple_key) != three_tuple_map.end()) {
      auto& endpoint_list = three_tuple_map[three_tuple_key];
      endpoint_list.insert(
          endpoint_list.end(), endpoints.begin(), endpoints.end());
    }
    else
      three_tuple_map[three_tuple_key] = endpoints;
  }
  return three_tuple_map;
}
