#include <iostream>

#include "src/dpdk_initial.h"
#include "src/initial_data_get.h"
#include "src/load_balance.h"

using namespace std;

int
main(int argc, char* argv[])
{
    if (rte_eal_init(argc, argv) < 0)
        rte_exit(EXIT_FAILURE, "Error with eal init\n");

    if (argc > 1) {
        g_if_test_latency = true;
    }
    auto nics_data_plane = f_nic_info_get_data_plane("192.168");
    g_nic_info_data_plane = nics_data_plane[0];
    auto nics_ctrl_plane = f_nic_info_get("192.168.255");
    g_nic_info_ctrl_plane = nics_ctrl_plane[0];
    g_controller_ip = ntohl(inet_addr("192.168.255.1"));
    g_virt_ip_prefix = ntohl(inet_addr("192.168.100.0"));
    g_virt_ip_mask = ntohl(inet_addr("255.255.255.0"));
    uint32_t queue_num_data_plane = 8;
    uint32_t queue_num_ctrl_plane = 1;

    g_map_three_tuple_endpoints = f_initial_data_get("config.json");
    for (uint32_t i = 0; i < queue_num_data_plane; i++) {
        FiveTupleToEndpointMap five_tuple_map;
        g_map_five_tuple_endpoint.push_back(five_tuple_map);
    }
    struct rte_mempool* mempool_ptr_data_plane = rte_pktmbuf_pool_create(
        "mempool_ptr_data", kMbufsNum, kMbufsCacheSize, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    struct rte_mempool* mempool_ptr_ctrl_plane = rte_pktmbuf_pool_create(
        "mempool_ptr_ctrl", kMbufsNum, kMbufsCacheSize, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    f_dpdk_nic_queue_initial(mempool_ptr_data_plane, g_nic_info_data_plane.id, queue_num_data_plane);
    f_dpdk_nic_queue_initial(mempool_ptr_ctrl_plane, g_nic_info_ctrl_plane.id, queue_num_ctrl_plane);

    struct QueueConfig* queue_config = new QueueConfig(mempool_ptr_ctrl_plane, 0);
    if (rte_eal_remote_launch(f_ctrl_pkts_update, queue_config, 1) < 0)
        rte_exit(EXIT_FAILURE, "Error launching thread on lcore %d\n", 1);

    for (uint32_t i = 0; i < queue_num_data_plane; i++) {
        struct QueueConfig* queue_config_data_plane = new QueueConfig(mempool_ptr_data_plane, i);
        if (rte_eal_remote_launch(f_load_balance, queue_config_data_plane, i + 2) < 0)
            rte_exit(EXIT_FAILURE, "Error launching thread on lcore %d\n", i + 2);
    }
    rte_eal_mp_wait_lcore();
    return 0;
}
