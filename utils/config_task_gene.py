from utils.config_job_gene import (
    f_bot_service_config_gene,
    f_latency_service_config_gene,
    f_load_cassandra_config_gene,
    f_load_cassandra_request_gene,
    f_load_iperf_request_config_gene,
    f_rate_request_config_gene,
    f_rate_service_config_gene,
)
from utils.manifest_utils import (
    f_map_sw_port_get,
    f_maps_local_ip_get,
)


def f_nodes_config_gene(lb_num, active_nodes):
    nodes_num = len(active_nodes) + 1
    required_nodes_num = lb_num * 2 + 2
    if nodes_num < required_nodes_num:
        print("The number of booked nodes is not enough," "please reduce the scale.")
        exit(1)
    controller_index = [1]
    lbs_index = [active_nodes[i] for i in range(0, lb_num)]
    backends_index = [active_nodes[i] for i in range(lb_num, 2 * lb_num)]
    clients_index = [active_nodes[i] for i in range(2 * lb_num, len(active_nodes))]
    nodes_config = {
        "controller": controller_index,
        "load_balancer": lbs_index,
        "backend": backends_index,
        "client": clients_index,
    }
    return nodes_config


def f_bot_nodes_config_gene(active_nodes):
    controller_index = [1]
    lbs_index = [active_nodes[0]]
    backends_index = [active_nodes[1], active_nodes[2]]
    clients_index = [active_nodes[3]]
    nodes_config = {
        "controller": controller_index,
        "load_balancer": lbs_index,
        "backend": backends_index,
        "client": clients_index,
    }
    return nodes_config


def f_sw_config_gene(xml_file_path, lbs_index):
    sw_config_cmds = []
    ctrl_plane_ips, data_plane_ips, sw_gateway_ips = f_maps_local_ip_get(xml_file_path)
    sw_ports = f_map_sw_port_get(xml_file_path)
    sw_config_cmds.append("enable")
    sw_config_cmds.append("configure")
    for node_name, sw_port in sw_ports.items():
        sw_config_cmds.append(f"default interface TenGigabitEthernet 1/{sw_port}")
        sw_config_cmds.append(f"interface TenGigabitEthernet 1/{sw_port}")
        sw_config_cmds.append("no shutdown")
        sw_config_cmds.append("no switchport")
        sw_config_cmds.append(f"ip address {sw_gateway_ips[node_name]}/24")
        sw_config_cmds.append("exit")
    for index in lbs_index:
        lb_name = f"node{index}"
        lb_ip_data_plane = data_plane_ips[lb_name]
        sw_config_cmds.append(
            f"ip route 192.168.100.0 255.255.255.0 {lb_ip_data_plane}"
        )
    print("Switch configuration:\n")
    for command in sw_config_cmds:
        print(command)
    print("\n")


def f_sw_clear_gene(xml_file_path, lbs_index):
    sw_config_cmds = []
    ctrl_plane_ips, data_plane_ips, sw_gateway_ips = f_maps_local_ip_get(xml_file_path)
    sw_ports = f_map_sw_port_get(xml_file_path)
    sw_config_cmds.append("enable")
    sw_config_cmds.append("configure")
    # for node_name, sw_port in sw_ports.items():
    #   sw_config_cmds.append(f"default interface TenGigabitEthernet 1/{sw_port}")
    #   sw_config_cmds.append(f"interface TenGigabitEthernet 1/{sw_port}")
    #   sw_config_cmds.append("no shutdown")
    #   sw_config_cmds.append("exit")
    for index in lbs_index:
        lb_name = f"node{index}"
        lb_ip_data_plane = data_plane_ips[lb_name]
        sw_config_cmds.append(
            f"no ip route 192.168.100.0 255.255.255.0 {lb_ip_data_plane}"
        )
    print("Switch configuration:\n")
    for command in sw_config_cmds:
        print(command)
    print("\n")


def f_rate_services_config_gene(backends_index, clients_index):
    services = []
    for backend_index in backends_index:
        node_name = "node" + str(backend_index)
        backend_ip = "192.168." + str(backend_index) + ".2"
        virt_ip = "192.168.100." + str(backend_index)
        backend_port = str(8000)
        virt_port = str(10000)
        protocol = "udp"
        # protocol = "tcp"
        service = f_rate_service_config_gene(
            backend_ip, backend_port, virt_ip, virt_port, protocol, node_name
        )
        services.append(service)

    requests = []
    for i in range(len(backends_index)):
        node_name = "node" + str(backends_index[i])
        backend_ip = "192.168." + str(backends_index[i]) + ".2"
        next_index = (i + 1) % len(backends_index)
        next_backend_index = backends_index[next_index]
        virt_ip = "192.168.100." + str(next_backend_index)
        virt_port = str(10000)
        protocol = "udp"
        request = f_rate_request_config_gene(
            backend_ip, virt_ip, virt_port, protocol, node_name
        )
        requests.append(request)

    client_request_num = min(len(backends_index), len(clients_index))
    for i in range(client_request_num):
        node_name = "node" + str(clients_index[i])
        client_ip = "192.168." + str(clients_index[i]) + ".2"
        virt_ip = "192.168.100." + str(backends_index[i])
        virt_port = str(10000)
        protocol = "udp"
        request = f_rate_request_config_gene(
            client_ip, virt_ip, virt_port, protocol, node_name
        )
        requests.append(request)
    return services, requests


def f_load_services_config_gene(backends_index, clients_index, load_type):
    services = []
    requests = []
    real_type, details = load_type.split("-")
    if real_type == "iperf":
        for backend_index in backends_index:
            node_name = "node" + str(backend_index)
            backend_ip = "192.168." + str(backend_index) + ".2"
            virt_ip = "192.168.100." + str(backend_index)
            backend_port = str(8000)
            virt_port = str(10000)
            protocol = "udp"
            service = f_rate_service_config_gene(
                backend_ip, backend_port, virt_ip, virt_port, protocol, node_name
            )
            services.append(service)
        for i in range(len(backends_index)):
            node_name = "node" + str(backends_index[i])
            backend_ip = "192.168." + str(backends_index[i]) + ".2"
            next_index = (i + 1) % len(backends_index)
            next_backend_index = backends_index[next_index]
            virt_ip = "192.168.100." + str(next_backend_index)
            virt_port = str(10000)
            protocol = "udp"
            request = f_load_iperf_request_config_gene(
                backend_ip, virt_ip, virt_port, protocol, node_name
            )
            requests.append(request)
    elif real_type == "cassandra":
        for backend_index in backends_index:
            node_name = "node" + str(backend_index)
            backend_ip = "192.168." + str(backend_index) + ".2"
            virt_ip = "192.168.100." + str(backend_index)
            backend_port = str(9042)
            virt_port = str(10000)
            service = f_load_cassandra_config_gene(
                backend_ip, backend_port, virt_ip, virt_port, backends_index, node_name
            )
            services.append(service)
        client_index = clients_index[0]
        backend_index = backends_index[0]
        node_name = "node" + str(client_index)
        virt_ip = "192.168.100." + str(backend_index)
        virt_port = str(10000)
        request = f_load_cassandra_request_gene(virt_ip, virt_port, node_name)
        requests.append(request)
    return services, requests


def f_latency_intra_services_gene(backends_index):
    services = []
    requests = []
    for backend_index in backends_index:
        node_name = "node" + str(backend_index)
        backend_ip = "192.168." + str(backend_index) + ".2"
        virt_ip = "192.168.100.1"
        backend_port = str(8000)
        virt_port = str(10000)
        protocol = "udp"
        service = f_latency_service_config_gene(
            backend_ip, backend_port, virt_ip, virt_port, protocol, node_name
        )
        services.append(service)
        break
    return services, requests


def f_bot_services_gene(backends_index):
    services = []
    requests = []
    index = 1
    for backend_index in backends_index:
        node_name = "node" + str(backend_index)
        backend_ip = "192.168." + str(backend_index) + ".2"
        virt_ip = "192.168.100." + str(index)
        backend_port = str(8000)
        virt_port = str(10000)
        protocol = "udp"
        service = f_bot_service_config_gene(
            backend_ip, backend_port, virt_ip, virt_port, protocol, node_name
        )
        services.append(service)
        index += 1
    return services, requests
