#include <cstring>
#include <iostream>
#include "src/user/bpf_prog_manage.h"
#include "src/user/client_self_define.h"

using namespace std;


int main(int argc, char *argv[]) {
  if (argc < 2) {
    cout << "Provide a request type as argument.\n";
    return -1;
  } 

  if (strcmp(argv[1], "intra") == 0) {
    string service_ip = "192.168.100.1";
    f_latency_test_client(service_ip, 10000);
  }
  else if (strcmp(argv[1], "inter") == 0) {
    f_latency_test_tcp_server(20000);
  }
  else if (strcmp(argv[1], "internet") == 0) {
    f_latency_test_tcp_server(20000);
  }
  else if (strcmp(argv[1], "bot") == 0) {
    if (argc < 3) {
      cout << "Provide a bot rate as argument.\n";
      return -1;
    }
    float bot_ratio = atof(argv[3]);
    if (strcmp(argv[2], "beaver") == 0) {
      f_bot_beaver_test(bot_ratio);
    }
    else if (strcmp(argv[2], "poll") == 0) {
      f_bot_poll_test(bot_ratio);
    }
    else if (strcmp(argv[2], "laiyang") == 0) {
      f_bot_laiyang_test(bot_ratio);
    }
    else {
      cout << "Invalid argument.\n";
      return -1;
    }
  }
  else {
    cout << "Invalid argument.\n";
    return -1;
  }
}


// int main(int argc, char *argv[]) {
//   if (argc < 2) {
//     cout << "Provide load or unload as argument.\n";
//     return -1;
//   }

//   int if_index = f_nic_ifindex_get("192.168");
//   int ret;
//   if (strcmp(argv[1], "load") == 0) {
//     ret = f_bpf_ingress_prog_load("src/kernel/client_snat.o", if_index);
//     if (ret) {
//       cout << "Error loading ingress program.\n";
//       return ret;
//     }
//     ret = f_bpf_egress_prog_load("src/kernel/lb_redirect.o", if_index);
//     if (ret) {
//       cout << "Error loading egress program.\n";
//       return -1;
//     }
//   } else if (strcmp(argv[1], "unload") == 0) {
//     ret = f_bpf_prog_unload(if_index);
//     if (ret) {
//       cout << "Error unloading egress program.\n";
//       return -1;
//     }
//   } else {
//     cout << "Invalid argument.\n";
//     return -1;
//   }
//   return 0;
// }