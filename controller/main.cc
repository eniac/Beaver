#include <cstdio>
#include <iostream>

#include <fcntl.h>
#include <sys/stat.h>

#include "src/globals.h"
#include "src/initial_data_get.h"
#include "src/nic_info_get.h"
#include "src/ss_control.h"

using namespace std;

int
main(int argc, char* argv[])
{
    if (rte_eal_init(argc, argv) < 0)
        rte_exit(EXIT_FAILURE, "Error with EAL init at main\n");
    g_max_ss_id = 1000;
    g_ss_freq = 100;
    auto nics_ctrl_plane = f_nic_info_get("192.168.255");
    g_nic_info_ctrl_plane = nics_ctrl_plane[0];
    g_lb_ips = f_lb_config_load("config.json");
    g_backend_ips = f_backend_config_load("config.json");
    // printf("g_nic_info_ctrl_plane.id: %d\n", g_nic_info_ctrl_plane.id);

    struct rte_mempool* mempool_ptr_ctrl_plane = rte_pktmbuf_pool_create(
        "mempool_ptr_ctrl", kMbufsNum, kMbufsCacheSize, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    f_dpdk_nic_queue_initial(mempool_ptr_ctrl_plane, g_nic_info_ctrl_plane.id);

    struct QueueConfig* queue_config = new QueueConfig(mempool_ptr_ctrl_plane, 0);

    if (rte_eal_remote_launch(f_ss_pkts_recv, queue_config, 2) < 0)
        rte_exit(EXIT_FAILURE, "Error launching thread on lcore %d\n", 2);

    struct QueueConfig* queue_config_virt_req = new QueueConfig(mempool_ptr_ctrl_plane, 1);
    if (rte_eal_remote_launch(f_virt_info_reqs_manage, queue_config_virt_req, 3) < 0) {
        rte_exit(EXIT_FAILURE, "Error launching thread on lcore %d\n", 3);
    }

    struct QueueConfig* queue_config_ss_tail = new QueueConfig(mempool_ptr_ctrl_plane, 2);
    if (rte_eal_remote_launch(f_ss_tail_pkts_recv, queue_config_ss_tail, 4) < 0) {
        rte_exit(EXIT_FAILURE, "Error launching thread on lcore %d\n", 4);
    }
    struct Data
    {
        int frequency;
        int count;
        int block;
    } data;
    const char* fifo_path = "/tmp/myfifo";
    mkfifo(fifo_path, 0666);
    int fifo_fd = open(fifo_path, O_RDONLY);
    if (fifo_fd == -1) {
        cout << "Failed to open FIFO\n";
        return 1;
    }
    if (read(fifo_fd, &data, sizeof(data)) > 0) {
        g_ss_freq = data.frequency;
        g_max_ss_id = data.count;
        g_block = data.block;
        if (rte_eal_remote_launch(f_ss_pkts_send, queue_config, 1) < 0)
            rte_exit(EXIT_FAILURE, "Error launching thread on lcore %d\n", 1);
    }
    close(fifo_fd);
    rte_eal_mp_wait_lcore();
    return 0;
}