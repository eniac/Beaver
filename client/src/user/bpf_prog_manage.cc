#include "bpf_prog_manage.h"

/*
 * @brief: load pinned object in our context.
 *         If we have pinned map "xx" to "/sys/fs/bpf/xx",
 *         we can load it by f_bpf_obj_get("/sys/fs/bpf/xx")
 * @param: the path of pinned object (like "/sys/fs/bpf/xx")
 */
int
f_bpf_obj_get(const char* pathname)
{
    union bpf_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.pathname = (__u64)(unsigned long)((void*)pathname);
    return f_bpf_syscall(BPF_OBJ_GET, &attr, sizeof(attr));
}

int
f_bpf_ingress_prog_load(const char* prog_pathname, int if_index)
{
    int prog_fd, ret;
    struct bpf_object* prog_obj = bpf_object__open(prog_pathname);
    if (prog_obj == NULL) {
        printf("Error loading bpf object.\n");
        return -1;
    }

    struct bpf_map* virt_ip_hash_table = bpf_object__find_map_by_name(prog_obj, "virt_ip_hash_table");
    if (virt_ip_hash_table == NULL) {
        printf("Error finding map virt_ip_hash_table.\n");
        return -1;
    }

    struct bpf_program* prog = bpf_object__find_program_by_name(prog_obj, "f_client_snat");
    if (prog == NULL) {
        printf("Error finding program.\n");
        return -1;
    }
    ret = bpf_program__set_xdp(prog);
    if (ret) {
        printf("Error setting program as xdp.\n");
        return -1;
    }
    ret = bpf_object__load(prog_obj);
    if (ret) {
        printf("Error loading bpf object file.\n");
        return ret;
    }

    ret = bpf_map__pin(virt_ip_hash_table, "/sys/fs/bpf/virt_ip_hash_table");
    if (ret) {
        printf("Error pinning map virt_ip_hash_table.\n");
        return ret;
    }

    prog_fd = bpf_program__fd(prog);
    if (prog_fd < 0) {
        printf("Error getting program fd.\n");
        return -1;
    }
    ret = bpf_set_link_xdp_fd(if_index, prog_fd, XDP_FLAGS_DRV_MODE);
    if (ret) {
        printf("Error attaching XDP program.\n");
        bpf_object__close(prog_obj);
        return ret;
    }
    bpf_object__close(prog_obj);
    return 0;
}

int
f_bpf_egress_prog_load(const char* prog_pathname, int if_index)
{
    int prog_fd, ret;
    int virt_ip_hash_table_fd;
    struct bpf_object* prog_obj = bpf_object__open(prog_pathname);
    if (prog_obj == NULL) {
        printf("Error loading bpf object.\n");
        return -1;
    }

    struct bpf_map* lb_ip_array = bpf_object__find_map_by_name(prog_obj, "lb_ip_array");
    if (lb_ip_array == NULL) {
        printf("Error finding map.\n");
        return -1;
    }
    struct bpf_map* virt_ip_hash_table = bpf_object__find_map_by_name(prog_obj, "virt_ip_hash_table");
    if (virt_ip_hash_table == NULL) {
        printf("Error finding map virt_ip_hash_table.\n");
        return -1;
    }

    struct bpf_program* prog = bpf_object__find_program_by_name(prog_obj, "f_lb_redirect");
    if (prog == NULL) {
        printf("Error finding program.\n");
        return -1;
    }
    ret = bpf_program__set_sched_cls(prog);
    if (ret) {
        printf("Error setting program as sched cls.\n");
        return -1;
    }

    virt_ip_hash_table_fd = f_bpf_obj_get("/sys/fs/bpf/virt_ip_hash_table");
    if (virt_ip_hash_table_fd < 0) {
        printf("Error getting virt_ip_hash_table fd.\n");
        return -1;
    }

    if (bpf_map__reuse_fd(virt_ip_hash_table, virt_ip_hash_table_fd)) {
        printf("Error reusing virt_ip_hash_table fd.\n");
        return -1;
    }

    ret = bpf_object__load(prog_obj);
    if (ret) {
        printf("Error loading bpf object file.\n");
        return ret;
    }

    ret = bpf_map__pin(lb_ip_array, "/sys/fs/bpf/lb_ip_array");
    if (ret) {
        printf("Error pinning map lb_ip_array.\n");
        return ret;
    }

    prog_fd = bpf_program__fd(prog);
    if (prog_fd < 0) {
        printf("Error getting program fd.\n");
        return -1;
    }

    DECLARE_LIBBPF_OPTS(bpf_tc_hook, tc_hook, .ifindex = if_index, .attach_point = BPF_TC_EGRESS);
    DECLARE_LIBBPF_OPTS(bpf_tc_opts, tc_opts, .flags = BPF_TC_F_REPLACE, .handle = 1, .priority = 1);

    ret = bpf_tc_hook_create(&tc_hook);
    if (ret) {
        printf("Error creating hook.\n");
        return ret;
    }
    tc_opts.prog_fd = prog_fd;

    ret = bpf_tc_attach(&tc_hook, &tc_opts);
    if (ret) {
        printf("Error attaching program.\n");
        bpf_object__close(prog_obj);
        return ret;
    }
    bpf_object__close(prog_obj);
    return 0;
}

int
f_bpf_prog_unload(int if_index)
{
    int ret;
    if (unlink("/sys/fs/bpf/lb_ip_array")) {
        printf("Error unlinking map.\n");
        return -1;
    }
    if (unlink("/sys/fs/bpf/virt_ip_hash_table")) {
        printf("Error unlinking map.\n");
        return -1;
    }

    if (bpf_set_link_xdp_fd(if_index, -1, XDP_FLAGS_DRV_MODE)) {
        printf("Error detaching XDP program.\n");
        return -1;
    }
    DECLARE_LIBBPF_OPTS(
        bpf_tc_hook,
        tc_hook,
        .ifindex = if_index,
        .attach_point = static_cast<bpf_tc_attach_point>(BPF_TC_EGRESS | BPF_TC_INGRESS));

    if (bpf_tc_hook_destroy(&tc_hook) < 0) {
        printf("Error destroying tc hook.\n");
        return -1;
    }
    return 0;
}

/*
 * @brief: Get the interface index by ip prefix
 */
int
f_nic_ifindex_get(const char* ip_prefix)
{
    int socket_fd, if_index = -1;
    struct ifconf interface_config;
    struct ifreq *interface_requests, *current_interface;
    char buffer[4096];
    int num_interfaces;

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1) {
        printf("Error initialize socket.\n");
        return -1;
    }

    interface_config.ifc_len = sizeof(buffer);
    interface_config.ifc_buf = buffer;
    if (ioctl(socket_fd, SIOCGIFCONF, &interface_config) == -1) {
        printf("Error getting interface config.\n");
        close(socket_fd);
        return -1;
    }

    interface_requests = interface_config.ifc_req;
    num_interfaces = interface_config.ifc_len / sizeof(struct ifreq);
    for (uint32_t i = 0; i < num_interfaces; i++) {
        struct sockaddr_in* interface_address;
        current_interface = &interface_requests[i];
        interface_address = (struct sockaddr_in*)&current_interface->ifr_addr;
        char* interface_ip = inet_ntoa(interface_address->sin_addr);
        if (strstr(interface_ip, ip_prefix) == interface_ip) {
            if_index = if_nametoindex(current_interface->ifr_name);
            break;
        }
    }
    close(socket_fd);
    return if_index;
}