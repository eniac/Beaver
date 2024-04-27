#include "endpoint_hash.h"

std::shared_mutex g_lock_three_tuple_endpoints;
std::vector<FiveTupleToEndpointMap> g_map_five_tuple_endpoint;
ThreeTupleToEndpointsMap g_map_three_tuple_endpoints;

struct PackedEndpoint
f_endpoint_hash(FiveTupleToEndpointMap& five_tuple_map, const PackedFiveTupleKey& five_tuple_key)
{
    FiveTupleToEndpointMap::iterator five_tuple_map_iter = five_tuple_map.find(five_tuple_key);
    if (five_tuple_map_iter != five_tuple_map.end())
        return five_tuple_map_iter->second;

    PackedThreeTupleKey three_tuple_key(five_tuple_key.dst_ip, five_tuple_key.dst_port, five_tuple_key.protocol);
    std::size_t hash_value, hash_index;
    PackedEndpoint endpoint;
    // printf("five_tuple_key: %x, %d\n", five_tuple_key.dst_ip, five_tuple_key.dst_port);
    {
        std::shared_lock<std::shared_mutex> lock(g_lock_three_tuple_endpoints);
        ThreeTupleToEndpointsMap::const_iterator three_tuple_map_iter =
            g_map_three_tuple_endpoints.find(three_tuple_key);
        if (three_tuple_map_iter != g_map_three_tuple_endpoints.end()) {
            hash_value = FiveTupleHash()(five_tuple_key);
            hash_index = hash_value % (three_tuple_map_iter->second.size());
            endpoint = three_tuple_map_iter->second[hash_index];
            five_tuple_map[five_tuple_key] = endpoint;
        }
    }
    return endpoint;
}
