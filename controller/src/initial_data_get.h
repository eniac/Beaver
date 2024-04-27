#ifndef INITIAL_DATA_GET_H_
#define INITIAL_DATA_GET_H_

#include <fstream>
#include <vector>

#include <arpa/inet.h>
#include <json/json.h>

std::vector<uint32_t>
f_lb_config_load(const std::string& file_path);

std::vector<uint32_t>
f_backend_config_load(const std::string& file_path);

#endif // INITIAL_DATA_GET_H_