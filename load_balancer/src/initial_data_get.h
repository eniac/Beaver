#ifndef INITIAL_DATA_GET_H_
#define INITIAL_DATA_GET_H_

#include <arpa/inet.h>
#include <fstream>
#include <json/json.h>

#include "endpoint_hash.h"

ThreeTupleToEndpointsMap
f_initial_data_get(const std::string& file_path);
#endif // INITIAL_DATA_GET_H_
