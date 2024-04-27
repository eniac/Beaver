#include "src/user/bpf_prog_manage.h"
#include "src/user/service_self_define.h"
#include "src/user/virt_info_req.h"
#include <cstring>
#include <iostream>
#include <string.h>
#include <thread>
#include <vector>

using namespace std;

int
main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Please provide arguments\\n");
        return 1;
    }

    int if_index_data_plane = f_nic_ifindex_get_data_plane("192.168");
    std::vector<std::thread> threads;
    int ret;

    if (strcmp(argv[1], "load") == 0) {
        ret = f_bpf_ingress_prog_load("src/kernel/nat.o", if_index_data_plane);
        if (ret) {
            printf("Error loading ingress program.\n");
            return ret;
        }
        ret = f_bpf_egress_prog_load("src/kernel/snat.o", if_index_data_plane);
        if (ret) {
            printf("Error loading egress program.\n");
            return ret;
        }
    } else if (strcmp(argv[1], "unload") == 0) {
        ret = f_bpf_prog_unload(if_index_data_plane);
        if (ret) {
            printf("Error detaching bpf program.\n");
            return ret;
        }
    } else if (strcmp(argv[1], "user") == 0) {
        threads.push_back(std::thread(f_virt_info_req, 60000));
        threads.push_back(std::thread(f_ss_tail_send, 50001, 50000));
        for (auto& thread : threads)
            thread.join();
    } else if (strcmp(argv[1], "service") == 0) {
        threads.push_back(std::thread(f_service_latency_test));
        for (auto& thread : threads)
            thread.join();
    } else if (strcmp(argv[1], "bot") == 0) {
        if (argc < 3) {
            initial_bot();
            threads.push_back(std::thread(f_service_bot_test));
        } else if (strcmp(argv[2], "poll") == 0) {
            threads.push_back(std::thread(f_service_bot_test));
        } else if (strcmp(argv[2], "laiyang") == 0) {
            threads.push_back(std::thread(f_service_bot_ly_test, argv[3]));
        } else {
            printf("Invalid argument.\n");
        }
        for (auto& thread : threads)
            thread.join();
    } else {
        printf("Invalid argument.\n");
    }

    return 0;
}