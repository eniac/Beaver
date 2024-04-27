import json
import os
import re
import threading
import time

from utils.job_gene import (
    f_bot_laiyang_prog_single_node,
    f_bot_poll_prog_single_node,
    f_bot_prog_single_node,
    f_cassandra_env_config_single_node,
    f_compile_single_node,
    f_config_send_single_node,
    f_folder_delete_single_node,
    f_folder_files_delete_single_node,
    f_folder_send_single_node,
    f_latency_prog_single_node,
    f_load_iperf_request_single_node,
    f_remove_cassandra_single_node,
    f_remove_prog_single_node,
    f_remove_rate_service_single_node,
    f_run_cassandra_single_node,
    f_run_prog_single_node,
    f_run_rate_request_single_node,
    f_run_rate_service_single_node,
    f_set_env_single_node,
)
from utils.manifest_utils import (
    f_map_ssh_ip_get,
)
from utils.remote_ctrl_utils import (
    f_cmds_exec,
    f_ssh_connection_create,
)


def f_get_route_info(ssh_connection):
    stdin, stdout, stderr = ssh_connection.exec_command("ip route")
    route_info = stdout.read().decode("utf-8")
    link_line = ""
    for line in route_info.splitlines():
        if "192.168" in line and "192.168.255" not in line:
            link_line = line
            break
    if "linkdown" in link_line:
        return False
    else:
        return True


# def f_active_nodes_get(xml_file_path, user_name, key_path):
#   ssh_ips = f_map_ssh_ip_get(xml_file_path)
#   active_nodes = []
#   for node_name, ssh_ip in ssh_ips.items():
#     if ('node' in node_name):
#       ssh_ip = ssh_ips[node_name]
#       ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
#       if_linkup = f_get_route_info(ssh_connection)
#       ssh_connection.close()
#       if if_linkup:
#         match = re.search(r'node(\d+)', node_name)
#         if match:
#           node_index = int(match.group(1))
#           if (node_index != 1):
#             active_nodes.append(int(match.group(1)))
#   active_nodes.sort()
#   return active_nodes


def f_active_nodes_get(xml_file_path, user_name, key_path):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    active_nodes = []
    threads = []
    lock = threading.Lock()

    def process_node(node_name, ssh_ip):
        ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
        if_linkup = f_get_route_info(ssh_connection)
        ssh_connection.close()
        if if_linkup:
            match = re.search(r"node(\d+)", node_name)
            if match:
                node_index = int(match.group(1))
                if node_index != 1:
                    with lock:
                        active_nodes.append(node_index)

    for node_name, ssh_ip in ssh_ips.items():
        if "node" in node_name:
            thread = threading.Thread(target=process_node, args=(node_name, ssh_ip))
            thread.start()
            threads.append(thread)
    for thread in threads:
        thread.join()
    active_nodes.sort()
    return active_nodes


def f_set_env(xml_file_path, user_name, key_path, env_install_file_path):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    print("Start configuring the environment for all the nodes.")
    for node_name, ssh_ip in ssh_ips.items():
        thread = threading.Thread(
            target=f_set_env_single_node,
            args=(ssh_ip, user_name, key_path, env_install_file_path),
        )
        threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    print("Finish configuring the environment for all the nodes.")


def f_folders_send(xml_file_path, user_name, key_path, base_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    # print("Start sending folders needed to all the nodes.")
    for node_type, nodes_index in nodes_config.items():
        for index in nodes_index:
            node_name = "node" + str(index)
            ssh_ip = ssh_ips[node_name]
            thread = threading.Thread(
                target=f_folder_send_single_node,
                args=(ssh_ip, user_name, key_path, base_path, node_type),
            )
            threads.append(thread)
    if "external" in ssh_ips.keys():
        ssh_ip = ssh_ips["external"]
        thread = threading.Thread(
            target=f_folder_send_single_node,
            args=(ssh_ip, user_name, key_path, base_path, "client"),
        )
        threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    # print("Have sent folders needed to all the nodes.")


def f_delete_folders(xml_file_path, user_name, key_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    print("Start deleting folders for all the nodes.")
    for node_type, nodes_index in nodes_config.items():
        for index in nodes_index:
            node_name = "node" + str(index)
            ssh_ip = ssh_ips[node_name]
            thread = threading.Thread(
                target=f_folder_delete_single_node,
                args=(ssh_ip, user_name, key_path, node_type),
            )
            threads.append(thread)
    if "external" in ssh_ips.keys():
        ssh_ip = ssh_ips["external"]
        thread = threading.Thread(
            target=f_folder_delete_single_node,
            args=(ssh_ip, user_name, key_path, "client"),
        )
        threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    print("Have deleted folders for all the nodes.")


def f_replace_folders(
    xml_file_path, user_name, key_path, config_file_path, folder_type
):
    try:
        with open(config_file_path, "r") as file:
            config_data = json.load(file)
    except FileNotFoundError:
        print("Error: The file was not found, please configure first.")
        exit(1)
    nodes_config = config_data["nodes"]
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    nodes_index = nodes_config[folder_type]
    base_path = os.path.dirname(config_file_path)
    threads = []
    print(f"Start replacing folders needed to {folder_type}s.")
    for index in nodes_index:
        node_name = "node" + str(index)
        ssh_ip = ssh_ips[node_name]
        thread = threading.Thread(
            target=f_folder_files_delete_single_node,
            args=(ssh_ip, user_name, key_path, folder_type),
        )
        threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    threads = []
    for index in nodes_index:
        node_name = "node" + str(index)
        ssh_ip = ssh_ips[node_name]
        thread = threading.Thread(
            target=f_folder_send_single_node,
            args=(ssh_ip, user_name, key_path, base_path, folder_type),
        )
        threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    remote_path = os.path.join("/users/", user_name)
    remote_path = os.path.join(remote_path, folder_type)
    remote_config_path = os.path.join(remote_path, "config.json")
    threads = []
    for index in nodes_index:
        node_name = "node" + str(index)
        ssh_ip = ssh_ips[node_name]
        thread = threading.Thread(
            target=f_config_send_single_node,
            args=(ssh_ip, user_name, key_path, config_file_path, remote_config_path),
        )
        threads.append(thread)
        thread = threading.Thread(
            target=f_compile_single_node,
            args=(ssh_ip, user_name, key_path, folder_type),
        )
        threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    print(f"Have replaced folders needed to {folder_type}s.")


def f_config_send(xml_file_path, user_name, key_path, base_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    local_config_path = os.path.join(base_path, "config.json")
    remote_base_path = os.path.join("/users/", user_name)
    # print("Start sending config file needed to all the nodes.")
    for node_type, nodes_index in nodes_config.items():
        for index in nodes_index:
            node_name = "node" + str(index)
            ssh_ip = ssh_ips[node_name]
            remote_path = os.path.join(remote_base_path, node_type)
            remote_config_path = os.path.join(remote_path, "config.json")
            thread = threading.Thread(
                target=f_config_send_single_node,
                args=(
                    ssh_ip,
                    user_name,
                    key_path,
                    local_config_path,
                    remote_config_path,
                ),
            )
            threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    # print("Have sent config file needed to all the nodes.")


def f_compile(xml_file_path, user_name, key_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    # print("Start compiling remote files.")
    for node_type, nodes_index in nodes_config.items():
        for index in nodes_index:
            node_name = "node" + str(index)
            ssh_ip = ssh_ips[node_name]
            thread = threading.Thread(
                target=f_compile_single_node,
                args=(ssh_ip, user_name, key_path, node_type),
            )
            threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    # print("Have compiled remote files.")


def f_rate_progs_run(xml_file_path, user_name, key_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    controller_threads = []
    # print("Start running programs.")
    for node_type, nodes_index in nodes_config.items():
        if node_type == "client":
            continue
        for index in nodes_index:
            node_name = "node" + str(index)
            ssh_ip = ssh_ips[node_name]
            thread = threading.Thread(
                target=f_run_prog_single_node,
                args=(ssh_ip, user_name, key_path, node_type),
            )
            if node_type == "controller":
                controller_threads.append(thread)
            else:
                threads.append(thread)
    for thread in controller_threads:
        thread.start()
    for thread in controller_threads:
        thread.join()
    time.sleep(1)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    # print("Have started all the programs.")


def f_latency_intra_progs_run(xml_file_path, user_name, key_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    client_threads = []
    # print("Start running programs.")
    for node_type, nodes_index in nodes_config.items():
        if node_type == "controller":
            continue
        for index in nodes_index:
            node_name = "node" + str(index)
            ssh_ip = ssh_ips[node_name]
            thread = threading.Thread(
                target=f_latency_prog_single_node,
                args=(ssh_ip, user_name, key_path, node_type, "intra"),
            )
            if node_type == "client":
                client_threads.append(thread)
            else:
                threads.append(thread)
            break
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    time.sleep(1)
    for thread in client_threads:
        thread.start()
    for thread in client_threads:
        thread.join()
    # print("Have started all the programs.")


def f_latency_internet_progs_run(xml_file_path, user_name, key_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    client_threads = []
    print("Start running programs.")
    for node_type, nodes_index in nodes_config.items():
        if node_type == "controller":
            continue
        for index in nodes_index:
            node_name = "node" + str(index)
            ssh_ip = ssh_ips[node_name]
            thread = threading.Thread(
                target=f_latency_prog_single_node,
                args=(ssh_ip, user_name, key_path, node_type, "internet"),
            )
            if node_type == "client":
                client_threads.append(thread)
            else:
                threads.append(thread)
            break
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    time.sleep(1)
    for thread in client_threads:
        thread.start()
    for thread in client_threads:
        thread.join()
    print("Have started all the programs.")


def f_bot_programs_run(xml_file_path, user_name, key_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    print("Start running programs.")
    for node_type, nodes_index in nodes_config.items():
        if node_type == "client":
            continue
        for index in nodes_index:
            node_name = "node" + str(index)
            ssh_ip = ssh_ips[node_name]
            thread = threading.Thread(
                target=f_bot_prog_single_node,
                args=(ssh_ip, user_name, key_path, node_type),
            )
            threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    print("Have started all the programs.")


def f_bot_poll_programs_run(xml_file_path, user_name, key_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    print("Start running programs.")
    for node_type, nodes_index in nodes_config.items():
        if node_type == "client":
            continue
        for index in nodes_index:
            node_name = "node" + str(index)
            ssh_ip = ssh_ips[node_name]
            thread = threading.Thread(
                target=f_bot_poll_prog_single_node,
                args=(ssh_ip, user_name, key_path, node_type),
            )
            threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    print("Have started all the programs.")


def f_bot_laiyang_programs_run(xml_file_path, user_name, key_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    backends_index = nodes_config["backend"]
    print("Start running programs.")
    for node_type, nodes_index in nodes_config.items():
        if node_type == "client":
            continue
        for index in nodes_index:
            node_name = "node" + str(index)
            ssh_ip = ssh_ips[node_name]
            thread = threading.Thread(
                target=f_bot_laiyang_prog_single_node,
                args=(ssh_ip, user_name, key_path, node_type, backends_index, index),
            )
            threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    print("Have started all the programs.")


def f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    # print("Start removing programs.")
    for node_type, nodes_index in nodes_config.items():
        for index in nodes_index:
            node_name = "node" + str(index)
            ssh_ip = ssh_ips[node_name]
            thread = threading.Thread(
                target=f_remove_prog_single_node,
                args=(ssh_ip, user_name, key_path, node_type),
            )
            threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    # print("Have removed all the programs.")


def f_latency_intra_progs_remove(xml_file_path, user_name, key_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    # print("Start removing programs.")
    for node_type, nodes_index in nodes_config.items():
        if node_type == "controller":
            continue
        for index in nodes_index:
            node_name = "node" + str(index)
            ssh_ip = ssh_ips[node_name]
            thread = threading.Thread(
                target=f_remove_prog_single_node,
                args=(ssh_ip, user_name, key_path, node_type),
            )
            threads.append(thread)
            break
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    # print("Have removed all the programs.")


def f_rate_services_deploy(xml_file_path, user_name, key_path, services):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    # print("Start launching services.")
    for service in services:
        node_name = service["node_name"]
        ssh_ip = ssh_ips[node_name]
        thread = threading.Thread(
            target=f_run_rate_service_single_node,
            args=(ssh_ip, user_name, key_path, service),
        )
        threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    # print("Have started all the services.")


def f_load_cassandra_deploy(xml_file_path, user_name, key_path, services):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    print("Start launching cassandra services.")
    for service in services:
        node_name = service["node_name"]
        ssh_ip = ssh_ips[node_name]
        thread = threading.Thread(
            target=f_run_cassandra_single_node,
            args=(ssh_ip, user_name, key_path, service),
        )
        threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    print("Have started all the cassandra services.")


def f_load_cassandra_intial(xml_file_path, user_name, key_path, backend_index):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    node_name = "node" + str(backend_index)
    ssh_ip = ssh_ips[node_name]
    print("Start initializing cassandra.")
    cmds = [
        f'cqlsh 192.168.{backend_index}.2 9042 -e "CREATE KEYSPACE ycsb WITH REPLICATION = '
        "{'class': 'SimpleStrategy', 'replication_factor': 3};"
        "USE ycsb;"
        "CREATE TABLE usertable ("
        "y_id varchar PRIMARY KEY,"
        "field0 varchar, field1 varchar,"
        "field2 varchar, field3 varchar,"
        "field4 varchar, field5 varchar,"
        "field6 varchar, field7 varchar,"
        'field8 varchar, field9 varchar);"'
    ]
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    f_cmds_exec(ssh_connection, cmds)
    ssh_connection.close()
    print("Finish initializing cassandra.")


def f_rate_requests_deploy(xml_file_path, user_name, key_path, requests):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    # print("Start launching requests.")
    for request in requests:
        node_name = request["node_name"]
        ssh_ip = ssh_ips[node_name]
        thread = threading.Thread(
            target=f_run_rate_request_single_node,
            args=(ssh_ip, user_name, key_path, request),
        )
        threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    # print("Have started all the requests.")


def f_load_iperf_requests_deploy(
    xml_file_path, user_name, key_path, requests, thread_num
):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    print("Start launching requests.")
    for request in requests:
        node_name = request["node_name"]
        ssh_ip = ssh_ips[node_name]
        thread = threading.Thread(
            target=f_load_iperf_request_single_node,
            args=(ssh_ip, user_name, key_path, request, thread_num),
        )
        threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    print("Have started all the requests.")


def f_load_cassandra_request(xml_file_path, user_name, key_path, requests, operation):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    request = requests[0]
    node_name = request["node_name"]
    ssh_ip = ssh_ips[node_name]
    virt_ip = request["virt_ip"]
    virt_port = request["virt_port"]
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    operation_count = 50000
    workload = "workloada"
    if operation == "rw":
        workload = "workloada"
    elif operation == "r":
        workload = "workloadb"
    elif operation == "s":
        workload = "workloade"
        operation_count = 10000
    cmds = [
        f"cd ycsb && python2 ./bin/ycsb load cassandra-cql "
        f"-p hosts={virt_ip} -p port={virt_port} -P workloads/{workload} "
        f"-p operationcount={operation_count} -threads 1"
    ]
    cmds.append(
        f"cd ycsb && python2 ./bin/ycsb run cassandra-cql "
        f"-p hosts={virt_ip} -p port={virt_port} -P workloads/{workload} "
        f"-p operationcount={operation_count} -threads 1 > ~/ycsb_result.txt"
    )
    f_cmds_exec(ssh_connection, cmds)
    ssh_connection.close()


def f_rate_services_remove(xml_file_path, user_name, key_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    backends_index = nodes_config["backend"]
    clients_index = nodes_config["client"]
    threads = []
    # print("Start removing requests and services.")
    for index in backends_index:
        node_name = "node" + str(index)
        ssh_ip = ssh_ips[node_name]
        thread = threading.Thread(
            target=f_remove_rate_service_single_node, args=(ssh_ip, user_name, key_path)
        )
        threads.append(thread)
    for index in clients_index:
        node_name = "node" + str(index)
        ssh_ip = ssh_ips[node_name]
        thread = threading.Thread(
            target=f_remove_rate_service_single_node, args=(ssh_ip, user_name, key_path)
        )
        threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    # print("Have removed requests and services.")


def f_load_cassandra_env_config(xml_file_path, user_name, key_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    print("Start configuring cassandra environment.")
    for node_type, nodes_index in nodes_config.items():
        if node_type == "load_balancer" or node_type == "controller":
            continue
        for index in nodes_index:
            node_name = "node" + str(index)
            ssh_ip = ssh_ips[node_name]
            thread = threading.Thread(
                target=f_cassandra_env_config_single_node,
                args=(ssh_ip, user_name, key_path, node_type),
            )
            threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    print("Have configured cassandra environment.")


def f_load_cassandra_remove(xml_file_path, user_name, key_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    threads = []
    backends_index = nodes_config["backend"]
    print("Start removing cassandra services.")
    for index in backends_index:
        node_name = "node" + str(index)
        ssh_ip = ssh_ips[node_name]
        thread = threading.Thread(
            target=f_remove_cassandra_single_node, args=(ssh_ip, user_name, key_path)
        )
        threads.append(thread)
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
