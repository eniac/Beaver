#include "virt_info_req.h"

/*
* @brief: Acquire virtual information from manager (default on port 60000).
*/
void f_virt_info_req(__u16 port) {
  int ret;
  int snat_info_fd = f_bpf_obj_get("/sys/fs/bpf/snat_info");
  if (snat_info_fd < 0) {
    printf("Error getting snat_info fd.\n");
    return;
  }
  int virt_info_reqs_fd = f_bpf_obj_get("/sys/fs/bpf/virt_info_reqs");
  if (virt_info_reqs_fd < 0) {
    printf("Error getting virt_info_reqs fd.\n");
    return;
  }
  int existing_reqs_fd = f_bpf_obj_get("/sys/fs/bpf/existing_reqs");
  if (existing_reqs_fd < 0) {
    printf("Error getting existing_reqs fd.\n");
    return;
  }

  int sock;
  struct sockaddr_in backend_addr, manager_addr;
  socklen_t addr_size;
  addr_size = sizeof(manager_addr);
  char buffer[1024];
  if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("Error creating socket.\n");
    return;
  }
  memset(&manager_addr, 0, sizeof(manager_addr));
  manager_addr.sin_family = AF_INET;
  manager_addr.sin_port = htons(port);
  manager_addr.sin_addr.s_addr = inet_addr(kControllerIp);
  memset(&backend_addr, 0, sizeof(backend_addr));
  backend_addr.sin_family = AF_INET;
  backend_addr.sin_port = htons(port);
  backend_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sock, (struct sockaddr *)&backend_addr, sizeof(backend_addr)) < 0) {
    printf("Error binding socket.\n");
    close(sock);
    return;
  }

  while (1) {
    struct PackedVirtInfoReq virt_info_req;
    ret = bpf_map_lookup_and_delete_elem(virt_info_reqs_fd, 
                                         NULL, &virt_info_req);
    if (ret != 0) 
      continue;
    struct PackedVirtInfo virt_info;
    ret = sendto(sock, &virt_info_req, sizeof(virt_info_req), 0, 
                 (struct sockaddr *)&manager_addr ,addr_size);
    if (ret < 0) {
      printf("Error sending request.\n");
      close(sock);
      break;
    }
    int recv_size = recvfrom(sock, &virt_info, sizeof(virt_info), 0, 
                             (struct sockaddr *)&manager_addr, &addr_size);
    if (recv_size > 0) {
      struct PackedPortProto port_proto;
      port_proto.port = virt_info_req.port;
      port_proto.protocol = virt_info_req.protocol;
      ret = bpf_map_update_elem(snat_info_fd, &port_proto, 
                                &virt_info, BPF_ANY);
      if (ret < 0) {
        printf("Error updating snat_info.\n");
        close(sock);
        break;
      }
      ret = bpf_map_delete_elem(existing_reqs_fd, &virt_info_req);
      if (ret < 0) {
        printf("Error deleting existing_reqs.\n");
        close(sock);
        break;
      }
    }
  }
  close(sock);
}


void f_ss_tail_send(uint16_t src_port, uint16_t dst_port) {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1) {
    printf("Socket creation failed at f_ss_tail_send.\n");
    return;
  }

  sockaddr_in controller_addr;
  controller_addr.sin_family = AF_INET;
  controller_addr.sin_port = htons(dst_port);
  inet_pton(AF_INET, kControllerIp, &controller_addr.sin_addr);

  int ss_id_key = 1;
  int ss_info_fd = f_bpf_obj_get("/sys/fs/bpf/ss_info");
  if (ss_info_fd < 0) {
    printf("Error getting ss_info fd\n");
    return;
  }
  while (true) {
    uint32_t ssid;
    if (bpf_map_lookup_elem(ss_info_fd, &ss_id_key, &ssid))
      continue;
    uint32_t ssid_net_order = htonl(ssid);
    sendto(sock, &ssid_net_order, sizeof(ssid_net_order), 0, 
           (struct sockaddr*)&controller_addr, sizeof(controller_addr));
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
  close(sock);
}