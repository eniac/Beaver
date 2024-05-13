import os
import re
import time

from utils.manifest_utils import (
    f_map_ssh_ip_get,
)
from utils.remote_cmd_gene import (
    f_cassandra_deploy_cmd,
    f_load_iperf_request_deploy_cmd,
    f_rate_request_deploy_cmd,
    f_rate_service_deploy_cmd,
)
from utils.remote_ctrl_utils import (
    f_cmds_exec,
    f_file_checker,
    f_file_remote_delete,
    f_file_remote_fetch,
    f_file_remote_send,
    f_folder_files_remote_delete,
    f_folder_remote_delete,
    f_folder_remote_send,
    f_ssh_connection_create,
)


def f_set_env_single_node(ssh_ip, user_name, key_path, env_install_file_path):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    remote_path = os.path.join("/users/", user_name)
    file_name = os.path.basename(env_install_file_path)
    remote_file_path = os.path.join(remote_path, file_name)
    f_file_remote_send(ssh_connection, env_install_file_path, remote_file_path)
    env_config_cmds = []
    env_config_cmds.append("chmod 777 " + file_name)
    env_config_cmds.append("./" + file_name)
    f_cmds_exec(ssh_connection, env_config_cmds)
    ssh_connection.close()


def f_folder_send_single_node(ssh_ip, user_name, key_path, base_path, node_type):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    remote_path = os.path.join("/users/", user_name)
    f_folder_remote_send(ssh_connection, base_path, remote_path, node_type)
    ssh_connection.close()


def f_folder_delete_single_node(ssh_ip, user_name, key_path, node_type):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    remote_path = os.path.join("/users/", user_name)
    remote_folder_path = os.path.join(remote_path, node_type)
    f_folder_remote_delete(ssh_connection, remote_folder_path)
    ssh_connection.close()


def f_folder_files_delete_single_node(ssh_ip, user_name, key_path, node_type):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    remote_path = os.path.join("/users/", user_name)
    remote_folder_path = os.path.join(remote_path, node_type)
    f_folder_files_remote_delete(ssh_connection, remote_folder_path)
    ssh_connection.close()


def f_config_send_single_node(
    ssh_ip, user_name, key_path, local_config_path, remote_config_path
):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    f_file_remote_send(ssh_connection, local_config_path, remote_config_path)
    ssh_connection.close()


def f_compile_single_node(ssh_ip, user_name, key_path, node_type):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    compile_cmds = ["cd " + node_type + " && make clean"]
    compile_cmds.append("cd " + node_type + " && make")
    f_cmds_exec(ssh_connection, compile_cmds)
    ssh_connection.close()


def f_run_prog_single_node(ssh_ip, user_name, key_path, node_type):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    if node_type == "backend":
        run_cmds = [
            "cd backend && make load",
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main user "
            "> /dev/null 2>&1 &",
        ]
    elif node_type == "controller":
        run_cmds = [
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main > controller.log 2>&1 &"
        ]
    else:
        run_cmds = [
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main > /dev/null 2>&1 &"
        ]
    f_cmds_exec(ssh_connection, run_cmds)
    ssh_connection.close()


def f_latency_prog_single_node(ssh_ip, user_name, key_path, node_type, latency_type):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    if node_type == "backend":
        run_cmds = [
            "cd backend && make load",
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main service "
            "> /dev/null 2>&1 &",
        ]
    elif node_type == "client":
        run_cmds = [
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main "
            f"{latency_type} > /dev/null 2>&1 &"
        ]
    elif node_type == "load_balancer":
        run_cmds = [
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main 1 > " "/dev/null 2>&1 &"
        ]
    f_cmds_exec(ssh_connection, run_cmds)
    ssh_connection.close()


def f_bot_prog_single_node(ssh_ip, user_name, key_path, node_type):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    if node_type == "backend":
        run_cmds = [
            "cd backend && make load",
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main user "
            "> /dev/null 2>&1 &",
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main bot " "> /dev/null 2>&1 &",
        ]
    elif node_type == "controller":
        run_cmds = [
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main > controller.log 2>&1 &"
        ]
    else:
        run_cmds = [
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main > /dev/null 2>&1 &"
        ]
    f_cmds_exec(ssh_connection, run_cmds)
    ssh_connection.close()


def f_bot_poll_prog_single_node(ssh_ip, user_name, key_path, node_type):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    if node_type == "backend":
        run_cmds = [
            "cd backend && make load",
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main user "
            "> /dev/null 2>&1 &",
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main bot poll "
            "> /dev/null 2>&1 &",
        ]
    else:
        run_cmds = [
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main > /dev/null 2>&1 &"
        ]
    f_cmds_exec(ssh_connection, run_cmds)
    ssh_connection.close()


def f_bot_laiyang_prog_single_node(
    ssh_ip, user_name, key_path, node_type, backends_index, real_index
):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    index = -1
    if node_type == "backend":
        index = backends_index.index(real_index)
    if node_type == "backend" and index == 1:
        run_cmds = [
            "cd backend && make load",
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main user "
            "> /dev/null 2>&1 &",
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main bot laiyang service "
            "> /dev/null 2>&1 &",
        ]
    elif node_type == "backend" and index == 0:
        service_index = backends_index[1]
        service_ip = "192.168." + str(service_index) + ".2"
        run_cmds = [
            "cd backend && make load",
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main bot laiyang "
            f"{service_ip} > /dev/null 2>&1 &",
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main user "
            "> /dev/null 2>&1 &",
        ]
    else:
        run_cmds = [
            f"cd {node_type} && nohup sudo stdbuf -o0 ./main > /dev/null 2>&1 &"
        ]
    f_cmds_exec(ssh_connection, run_cmds)
    ssh_connection.close()


def f_remove_prog_single_node(ssh_ip, user_name, key_path, node_type):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    cmds = [
        "ps aux | grep ./main | grep -v grep | awk '{{print $2}}' | xargs sudo kill -9"
    ]
    if node_type == "backend":
        cmds.append("cd backend && make unload")
    f_cmds_exec(ssh_connection, cmds)
    ssh_connection.close()


def f_run_rate_service_single_node(ssh_ip, user_name, key_path, service):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    service_cmds = f_rate_service_deploy_cmd(service)
    f_cmds_exec(ssh_connection, service_cmds)
    ssh_connection.close()


def f_run_cassandra_single_node(ssh_ip, user_name, key_path, service):
    if service["application"] != "cassandra":
        return
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    service_cmds = f_cassandra_deploy_cmd(service)
    f_cmds_exec(ssh_connection, service_cmds)
    ssh_connection.close()


def f_run_rate_request_single_node(ssh_ip, user_name, key_path, request):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    request_cmds = f_rate_request_deploy_cmd(request)
    f_cmds_exec(ssh_connection, request_cmds)
    ssh_connection.close()


def f_load_iperf_request_single_node(ssh_ip, user_name, key_path, request, thread_num):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    request_cmds = f_load_iperf_request_deploy_cmd(request, thread_num)
    f_cmds_exec(ssh_connection, request_cmds)
    ssh_connection.close()


def f_remove_rate_service_single_node(ssh_ip, user_name, key_path):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    cmds = [
        "ps aux | grep iperf | grep -v grep | awk '{{print $2}}' | xargs sudo kill -9"
    ]
    f_cmds_exec(ssh_connection, cmds)
    ssh_connection.close()


def f_cassandra_env_config_single_node(ssh_ip, user_name, key_path, node_type):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    cmds = []
    if node_type == "backend":
        cmds.append(
            'echo "deb https://debian.cassandra.apache.org 41x main" '
            "| sudo tee -a /etc/apt/sources.list.d/cassandra.sources.list"
        )
        cmds.append(
            "curl https://downloads.apache.org/cassandra/KEYS " "| sudo apt-key add -"
        )
        cmds.append("sudo apt-get update -y")
        cmds.append("sudo apt-get install cassandra cassandra-tools -y")
    elif node_type == "client":
        cmds.append(
            "curl -O --location https://github.com/brianfrankcooper"
            "/YCSB/releases/download/0.17.0/ycsb-0.17.0.tar.gz"
        )
        cmds.append("rm -rf ycsb")
        cmds.append("mkdir ycsb")
        cmds.append("tar xfvz ycsb-0.17.0.tar.gz -C ycsb --strip-components=1")
        cmds.append("rm -rf ycsb-0.17.0.tar.gz")
        cmds.append("sudo apt-get install -f python2 maven openjdk-8-jdk -y")
    f_cmds_exec(ssh_connection, cmds)
    ssh_connection.close()


def f_remove_cassandra_single_node(ssh_ip, user_name, key_path):
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    cmds = []
    cmds.append("sudo systemctl stop cassandra")
    f_cmds_exec(ssh_connection, cmds)
    ssh_connection.close()


def f_rate_ss_deploy(
    xml_file_path, user_name, key_path, nodes_config, if_parallel, frequency
):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    controller_index = nodes_config["controller"][0]
    controller_name = "node" + str(controller_index)
    ssh_ip = ssh_ips[controller_name]
    cmds = []
    if if_parallel:
        cmds.append(
            f"cd controller && nohup sudo stdbuf -o0 "
            f"./launch_ss -c 200000 -f {frequency} -b 0 2>&1 > /dev/null &"
        )
    else:
        cmds.append(
            f"cd controller && nohup sudo stdbuf -o0 "
            f"./launch_ss -c 20000 -f {frequency} -b 1 2>&1 > /dev/null &"
        )
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    # print("Start launching snapshots.")
    f_cmds_exec(ssh_connection, cmds)
    # print("Have started snapshots.")


def f_bound_ss_deploy(xml_file_path, user_name, key_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    controller_index = nodes_config["controller"][0]
    controller_name = "node" + str(controller_index)
    ssh_ip = ssh_ips[controller_name]
    cmds = []
    snapshot_num = 50000  # TODO(lc): Make the number of snapshots configurable.
    cmds.append(
        f"cd controller && nohup sudo stdbuf -o0 "
        f"./launch_ss -c {snapshot_num} -f 65000 -b 0 2>&1 > /dev/null &"
    )
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    # print("Start launching snapshots.")
    f_cmds_exec(ssh_connection, cmds)
    # print("Have started snapshots.")


def f_accuracy_ss_deploy(xml_file_path, user_name, key_path, nodes_config, frequency):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    controller_index = nodes_config["controller"][0]
    controller_name = "node" + str(controller_index)
    ssh_ip = ssh_ips[controller_name]
    cmds = []
    snapshot_num = frequency * 2
    cmds.append(
        f"cd controller && nohup sudo stdbuf -o0 "
        f"./launch_ss -c {snapshot_num} -f {frequency} -b 0 2>&1 > /dev/null &"
    )
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    # print("Start launching snapshots.")
    f_cmds_exec(ssh_connection, cmds)
    # print("Have started snapshots.")


def f_load_ss_deploy(xml_file_path, user_name, key_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    controller_index = nodes_config["controller"][0]
    controller_name = "node" + str(controller_index)
    ssh_ip = ssh_ips[controller_name]
    cmds = []
    cmds.append(
        f"cd controller && nohup sudo stdbuf -o0 "
        f"./launch_ss -c 20000000 -f 60000 -b 0 2>&1 > /dev/null &"
    )
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    print("Start launching snapshots.")
    f_cmds_exec(ssh_connection, cmds)
    print("Have started snapshots.")


def f_bot_ss_deploy(xml_file_path, user_name, key_path, nodes_config):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    controller_index = nodes_config["controller"][0]
    controller_name = "node" + str(controller_index)
    ssh_ip = ssh_ips[controller_name]
    cmds = []
    cmds.append(
        f"cd controller && nohup sudo stdbuf -o0 "
        f"./launch_ss -c 20000000 -f 5000 -b 2 2>&1 > /dev/null &"
    )
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    print("Start launching snapshots.")
    f_cmds_exec(ssh_connection, cmds)
    print("Have started snapshots.")


def f_rate_ss_result_pull(
    xml_file_path, user_name, key_path, nodes_config, local_file_path
):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    controller_index = nodes_config["controller"][0]
    controller_name = "node" + str(controller_index)
    ssh_ip = ssh_ips[controller_name]
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    remote_path = os.path.join("/users/", user_name)
    remote_path = os.path.join(remote_path, "controller")
    remote_file_path = os.path.join(remote_path, "freq_result.txt")
    if f_file_checker(ssh_connection, remote_file_path):
        f_file_remote_fetch(ssh_connection, remote_file_path, local_file_path)
        f_file_remote_delete(ssh_connection, remote_file_path)
        ssh_connection.close()
    else:
        ssh_connection.close()
        raise Exception("Fail to pull the result digest.")


def f_bound_ss_result_pull(
    xml_file_path, user_name, key_path, nodes_config, local_file_path
):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    controller_index = nodes_config["controller"][0]
    controller_name = "node" + str(controller_index)
    ssh_ip = ssh_ips[controller_name]
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    remote_path = os.path.join("/users/", user_name)
    remote_path = os.path.join(remote_path, "controller")
    remote_file_path = os.path.join(remote_path, "raw_data.txt")
    f_file_checker(ssh_connection, remote_file_path)
    time.sleep(2)
    f_file_remote_fetch(ssh_connection, remote_file_path, local_file_path)
    f_file_remote_delete(ssh_connection, remote_file_path)
    ssh_connection.close()


def f_accuracy_ss_result_pull(
    xml_file_path, user_name, key_path, nodes_config, local_file_path
):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    controller_index = nodes_config["controller"][0]
    controller_name = "node" + str(controller_index)
    ssh_ip = ssh_ips[controller_name]
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    remote_path = os.path.join("/users/", user_name)
    remote_path = os.path.join(remote_path, "controller")
    remote_file_path = os.path.join(remote_path, "freq_result.txt")
    f_file_checker(ssh_connection, remote_file_path)
    f_file_remote_fetch(ssh_connection, remote_file_path, local_file_path)
    f_file_remote_delete(ssh_connection, remote_file_path)
    ssh_connection.close()


def f_latency_intra_result_pull(
    xml_file_path, user_name, key_path, nodes_config, local_file_path
):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    load_balancer_index = nodes_config["load_balancer"][0]
    load_balancer_name = "node" + str(load_balancer_index)
    ssh_ip = ssh_ips[load_balancer_name]
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    remote_path = os.path.join("/users/", user_name)
    remote_path = os.path.join(remote_path, "load_balancer")
    remote_file_path = os.path.join(remote_path, "latency.txt")
    f_file_checker(ssh_connection, remote_file_path)
    time.sleep(1)
    f_file_remote_fetch(ssh_connection, remote_file_path, local_file_path)
    f_file_remote_delete(ssh_connection, remote_file_path)
    ssh_connection.close()


def f_bot_result_pull(
    xml_file_path, user_name, key_path, nodes_config, local_file_path
):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    client_index = nodes_config["client"][0]
    client_name = "node" + str(client_index)
    ssh_ip = ssh_ips[client_name]
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    remote_path = os.path.join("/users/", user_name)
    remote_path = os.path.join(remote_path, "client")
    remote_file_path = os.path.join(remote_path, "bot.txt")
    f_file_checker(ssh_connection, remote_file_path)
    time.sleep(1)
    f_file_remote_fetch(ssh_connection, remote_file_path, local_file_path)
    f_file_remote_delete(ssh_connection, remote_file_path)
    ssh_connection.close()


def f_load_cassandra_result_pull(
    xml_file_path, user_name, key_path, nodes_config, local_file_path
):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    client_index = nodes_config["client"][0]
    client_name = "node" + str(client_index)
    ssh_ip = ssh_ips[client_name]
    ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
    remote_path = os.path.join("/users/", user_name)
    remote_file_path = os.path.join(remote_path, "ycsb_result.txt")
    f_file_remote_fetch(ssh_connection, remote_file_path, local_file_path)
    f_file_remote_delete(ssh_connection, remote_file_path)
    ssh_connection.close()


def f_load_iperf_result_pull(
    xml_file_path, user_name, key_path, nodes_config, local_file_prefix
):
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    backends_index = nodes_config["backend"]
    for index in backends_index:
        node_name = "node" + str(index)
        ssh_ip = ssh_ips[node_name]
        ssh_connection = f_ssh_connection_create(ssh_ip, user_name, key_path)
        remote_path = os.path.join("/users/", user_name)
        remote_file_path = os.path.join(remote_path, "iperf_output.txt")
        local_file_name = local_file_prefix + "_" + node_name + ".txt"
        f_file_remote_fetch(ssh_connection, remote_file_path, local_file_name)
        f_file_remote_delete(ssh_connection, remote_file_path)
        ssh_connection.close()


def f_load_type_verify(load_type):
    iperf_pattern = r"^iperf-(10|20|30|40|50|60|70|80)$"
    cassandra_pattern = r"^cassandra-(rw|r|s)$"
    if re.match(iperf_pattern, load_type):
        return
    elif re.match(cassandra_pattern, load_type):
        return
    else:
        print("Invalid load type format")
        exit(1)
