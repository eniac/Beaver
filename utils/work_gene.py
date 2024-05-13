import datetime
import json
import os
import time

from utils.config_gene import (
    f_bot_config_gene,
    f_latency_intra_config_gene,
    f_load_config_gene,
    f_rate_config_gene,
)
from utils.config_task_gene import (
    f_sw_clear_gene,
    f_sw_config_gene,
)
from utils.job_gene import (
    f_accuracy_ss_deploy,
    f_accuracy_ss_result_pull,
    f_bot_result_pull,
    f_bot_ss_deploy,
    f_bound_ss_deploy,
    f_bound_ss_result_pull,
    f_latency_intra_result_pull,
    f_load_cassandra_result_pull,
    f_load_iperf_result_pull,
    f_load_ss_deploy,
    f_rate_ss_deploy,
    f_rate_ss_result_pull,
)
from utils.manifest_utils import (
    f_map_ssh_ip_get,
)
from utils.remote_ctrl_utils import (
    f_cmds_exec,
    f_ssh_connection_create,
)
from utils.results_handler import (
    f_average_bandwidth,
    f_print_file_contents,
)
from utils.task_gene import (
    f_active_nodes_get,
    f_bot_laiyang_programs_run,
    f_bot_poll_programs_run,
    f_bot_programs_run,
    f_compile,
    f_config_send,
    f_delete_folders,
    f_folders_send,
    f_latency_internet_progs_run,
    f_latency_intra_progs_remove,
    f_latency_intra_progs_run,
    f_load_cassandra_deploy,
    f_load_cassandra_env_config,
    f_load_cassandra_intial,
    f_load_cassandra_remove,
    f_load_cassandra_request,
    f_load_iperf_requests_deploy,
    f_rate_progs_remove,
    f_rate_progs_run,
    f_rate_requests_deploy,
    f_rate_services_deploy,
    f_rate_services_remove,
)


def f_rate_config(xml_file_path, user_name, key_path, lb_num, base_path):
    config_file_path = os.path.join(base_path, "config.json")
    active_nodes = f_active_nodes_get(xml_file_path, user_name, key_path)
    nodes_config = f_rate_config_gene(lb_num, config_file_path, active_nodes)
    f_folders_send(xml_file_path, user_name, key_path, base_path, nodes_config)
    f_config_send(xml_file_path, user_name, key_path, base_path, nodes_config)
    f_compile(xml_file_path, user_name, key_path, nodes_config)
    f_sw_config_gene(xml_file_path, nodes_config["load_balancer"])
    print(
        "Finished all the configurations.\n"
        "Please copy the commands above to the CloudLab switch console."
    )


def f_load_config(xml_file_path, user_name, key_path, lb_num, load_type, base_path):
    config_file_path = os.path.join(base_path, "config.json")
    active_nodes = f_active_nodes_get(xml_file_path, user_name, key_path)
    nodes_config = f_load_config_gene(lb_num, load_type, config_file_path, active_nodes)
    if "cassandra" in load_type:
        f_load_cassandra_env_config(xml_file_path, user_name, key_path, nodes_config)
    f_folders_send(xml_file_path, user_name, key_path, base_path, nodes_config)
    f_config_send(xml_file_path, user_name, key_path, base_path, nodes_config)
    f_compile(xml_file_path, user_name, key_path, nodes_config)
    f_sw_config_gene(xml_file_path, nodes_config["load_balancer"])
    print(
        "Have printed the switch configurations,"
        "please input all of them into the consoles of switch."
    )


def f_latency_intra_config(xml_file_path, user_name, key_path, base_path):
    config_file_path = os.path.join(base_path, "config.json")
    active_nodes = f_active_nodes_get(xml_file_path, user_name, key_path)
    nodes_config = f_latency_intra_config_gene(config_file_path, active_nodes)
    f_folders_send(xml_file_path, user_name, key_path, base_path, nodes_config)
    f_config_send(xml_file_path, user_name, key_path, base_path, nodes_config)
    f_compile(xml_file_path, user_name, key_path, nodes_config)
    f_sw_config_gene(xml_file_path, nodes_config["load_balancer"])
    print(
        "Have printed the switch configurations,"
        "please input all of them into the consoles of switch."
    )


def f_bot_config(xml_file_path, user_name, key_path, base_path):
    config_file_path = os.path.join(base_path, "config.json")
    active_nodes = f_active_nodes_get(xml_file_path, user_name, key_path)
    nodes_config = f_bot_config_gene(config_file_path, active_nodes)
    f_folders_send(xml_file_path, user_name, key_path, base_path, nodes_config)
    f_config_send(xml_file_path, user_name, key_path, base_path, nodes_config)
    f_compile(xml_file_path, user_name, key_path, nodes_config)
    f_sw_config_gene(xml_file_path, nodes_config["load_balancer"])
    print(
        "Have printed the switch configurations,"
        "please input all of them into the consoles of switch."
    )


def f_latency_inter_config(xml_file_path, user_name, key_path, base_path):
    config_file_path = os.path.join(base_path, "config.json")
    active_nodes = f_active_nodes_get(xml_file_path, user_name, key_path)
    nodes_config = f_latency_intra_config_gene(config_file_path, active_nodes)
    f_folders_send(xml_file_path, user_name, key_path, base_path, nodes_config)
    f_config_send(xml_file_path, user_name, key_path, base_path, nodes_config)
    f_compile(xml_file_path, user_name, key_path, nodes_config)
    f_sw_config_gene(xml_file_path, nodes_config["load_balancer"])
    print(
        "Have printed the switch configurations,"
        "please input all of them into the consoles of switch."
    )


def f_rate_clear(xml_file_path, user_name, key_path, config_file_path):
    try:
        with open(config_file_path, "r") as file:
            config_data = json.load(file)
    except FileNotFoundError:
        print("Error: The file was not found, please configure first.")
        exit(1)
    nodes_config = config_data["nodes"]
    f_delete_folders(xml_file_path, user_name, key_path, nodes_config)
    f_sw_clear_gene(xml_file_path, nodes_config["load_balancer"])
    print(
        "Have printed the configurations to clear switch,"
        "please input all of them into the consoles of switch."
    )


def f_load_clear(xml_file_path, user_name, key_path, config_file_path):
    try:
        with open(config_file_path, "r") as file:
            config_data = json.load(file)
    except FileNotFoundError:
        print("Error: The file was not found, please configure first.")
        exit(1)
    nodes_config = config_data["nodes"]
    f_delete_folders(xml_file_path, user_name, key_path, nodes_config)
    f_sw_clear_gene(xml_file_path, nodes_config["load_balancer"])
    print(
        "Have printed the configurations to clear switch,"
        "please input all of them into the consoles of switch."
    )


def f_rate_run(
    xml_file_path, user_name, key_path, config_file_path, if_parallel, frequency
):
    try:
        with open(config_file_path, "r") as file:
            config_data = json.load(file)
    except FileNotFoundError:
        print("Error: The file was not found, please configure first.")
        exit(1)
    nodes_config = config_data["nodes"]
    services = config_data["services"]
    requests = config_data["requests"]
    scale = len(nodes_config["load_balancer"])
    print("Start to run the experiment!")
    f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config)
    f_rate_services_remove(xml_file_path, user_name, key_path, nodes_config)
    f_rate_progs_run(xml_file_path, user_name, key_path, nodes_config)
    f_rate_services_deploy(xml_file_path, user_name, key_path, services)
    f_rate_requests_deploy(xml_file_path, user_name, key_path, requests)
    time.sleep(5)
    f_rate_ss_deploy(
        xml_file_path, user_name, key_path, nodes_config, if_parallel, frequency
    )
    time_name = datetime.datetime.now()
    timestamp = time_name.strftime("%Y%m%d%H%M%S")
    real_scale = scale
    file_name = f"freq_{real_scale}_{timestamp}.txt"
    if if_parallel:
        file_name = f"freq_para_{real_scale}_{timestamp}.txt"
    local_file_path = os.path.join("results/freq/", file_name)
    try:
        f_rate_ss_result_pull(
            xml_file_path, user_name, key_path, nodes_config, local_file_path
        )
        print("")
        f_print_file_contents(local_file_path)
        print("")
        print(
            f"Have finished the experiment, the results for Fig.10 have been outputed \n"
            f"the results for Fig.10 are also in file results/freq/{file_name}."
        )        
    except Exception as e:
        print("Catched an exception when pulling the results: {}".format(e))
    finally:
        f_rate_services_remove(xml_file_path, user_name, key_path, nodes_config)
        f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config)


def f_bound_run(xml_file_path, user_name, key_path, config_file_path):
    try:
        with open(config_file_path, "r") as file:
            config_data = json.load(file)
    except FileNotFoundError:
        print("Error: The file was not found, please configure first.")
        exit(1)
    nodes_config = config_data["nodes"]
    services = config_data["services"]
    requests = config_data["requests"]
    scale = len(nodes_config["load_balancer"])
    print("Start to run the experiment!")
    f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config)
    f_rate_services_remove(xml_file_path, user_name, key_path, nodes_config)
    f_rate_progs_run(xml_file_path, user_name, key_path, nodes_config)
    f_rate_services_deploy(xml_file_path, user_name, key_path, services)
    f_rate_requests_deploy(xml_file_path, user_name, key_path, requests)
    time.sleep(5)
    f_bound_ss_deploy(xml_file_path, user_name, key_path, nodes_config)
    time_name = datetime.datetime.now()
    timestamp = time_name.strftime("%Y%m%d%H%M%S")
    real_scale = scale
    file_name = f"bound_{real_scale}_{timestamp}.txt"
    local_file_path = os.path.join("results/bound/", file_name)
    try:
        f_bound_ss_result_pull(
            xml_file_path, user_name, key_path, nodes_config, local_file_path
        )
    except Exception as e:
        print("Catched an exception when pulling the results: {}".format(e))
    finally:
        print("Kill processes...")
        f_rate_services_remove(xml_file_path, user_name, key_path, nodes_config)
        f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config)
    print(
        f"Have finished the experiment.\n"
        f"Due the large number of results, the results are not printed.\n"
        f"the results for Fig.13 are in file results/bound/{file_name}."
    )


def f_accuracy_run(xml_file_path, user_name, key_path, config_file_path, frequency):
    try:
        with open(config_file_path, "r") as file:
            config_data = json.load(file)
    except FileNotFoundError:
        print("Error: The file was not found, please configure first.")
        exit(1)
    nodes_config = config_data["nodes"]
    services = config_data["services"]
    requests = config_data["requests"]
    scale = len(nodes_config["load_balancer"])
    print("Start to run the experiment!")
    f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config)
    f_rate_services_remove(xml_file_path, user_name, key_path, nodes_config)
    f_rate_progs_run(xml_file_path, user_name, key_path, nodes_config)
    f_rate_services_deploy(xml_file_path, user_name, key_path, services)
    f_rate_requests_deploy(xml_file_path, user_name, key_path, requests)
    time.sleep(5)
    f_accuracy_ss_deploy(xml_file_path, user_name, key_path, nodes_config, frequency)
    time_name = datetime.datetime.now()
    timestamp = time_name.strftime("%Y%m%d%H%M%S")
    real_scale = scale
    file_name = f"freq_{frequency}_{real_scale}_{timestamp}.txt"
    local_file_path = os.path.join("results/accuracy/", file_name)
    f_accuracy_ss_result_pull(
        xml_file_path, user_name, key_path, nodes_config, local_file_path
    )
    f_rate_services_remove(xml_file_path, user_name, key_path, nodes_config)
    f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config)
    print("")
    f_print_file_contents(local_file_path)
    print("")
    print(
        f"Have finished the experiment, the results for Fig.11 have been outputed \n"
        f"the results for Fig.11 are also in file results/accuracy/{file_name}."
    )


def f_load_run(
    xml_file_path, user_name, key_path, load_type, if_snapshot, config_file_path
):
    try:
        with open(config_file_path, "r") as file:
            config_data = json.load(file)
    except FileNotFoundError:
        print("Error: The file was not found, please configure first.")
        exit(1)
    nodes_config = config_data["nodes"]
    services = config_data["services"]
    requests = config_data["requests"]
    scale = len(nodes_config["load_balancer"])
    real_type, details = load_type.split("-")
    if real_type == "iperf":
        details = int(details) // 10
        f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config)
        f_rate_services_remove(xml_file_path, user_name, key_path, nodes_config)
        f_rate_progs_run(xml_file_path, user_name, key_path, nodes_config)
        f_rate_services_deploy(xml_file_path, user_name, key_path, services)
        time.sleep(5)
        if if_snapshot:
            f_load_ss_deploy(xml_file_path, user_name, key_path, nodes_config)
        f_load_iperf_requests_deploy(
            xml_file_path, user_name, key_path, requests, details
        )
        time.sleep(45)
        real_scale = scale
        time_name = datetime.datetime.now()
        timestamp = time_name.strftime("%Y%m%d%H%M%S")
        file_name_prefix = f"iperf_{real_scale}_{details}_{timestamp}"
        if if_snapshot:
            file_name_prefix = f"iperf_ss_{real_scale}_{details}_{timestamp}"
        local_file_prefix = os.path.join("results/load/", file_name_prefix)
        f_load_iperf_result_pull(
            xml_file_path, user_name, key_path, nodes_config, local_file_prefix
        )
        f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config)
        f_rate_services_remove(xml_file_path, user_name, key_path, nodes_config)
        f_average_bandwidth("results/load/", timestamp, file_name_prefix)
    elif real_type == "cassandra":
        f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config)
        f_load_cassandra_remove(xml_file_path, user_name, key_path, nodes_config)
        f_rate_progs_run(xml_file_path, user_name, key_path, nodes_config)
        f_load_cassandra_deploy(xml_file_path, user_name, key_path, services)
        time.sleep(40)
        backend_index = nodes_config["backend"][0]
        f_load_cassandra_intial(xml_file_path, user_name, key_path, backend_index)
        time.sleep(10)
        if if_snapshot:
            f_load_ss_deploy(xml_file_path, user_name, key_path, nodes_config)
        f_load_cassandra_request(xml_file_path, user_name, key_path, requests, details)
        time_name = datetime.datetime.now()
        timestamp = time_name.strftime("%Y%m%d%H%M%S")
        real_scale = scale
        file_name = f"ycsb_{real_scale}_{details}_{timestamp}.txt"
        if if_snapshot:
            file_name = f"ycsb_ss_{real_scale}_{details}_{timestamp}.txt"
        local_file_path = os.path.join("results/load/", file_name)
        f_load_cassandra_result_pull(
            xml_file_path, user_name, key_path, nodes_config, local_file_path
        )
        f_load_cassandra_remove(xml_file_path, user_name, key_path, nodes_config)
        f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config)
    print("Have finished the experiment, check folder " "results/load/ for results!")


def f_latency_intra_run(xml_file_path, user_name, key_path, config_file_path):
    try:
        with open(config_file_path, "r") as file:
            config_data = json.load(file)
    except FileNotFoundError:
        print("Error: The file was not found, please configure first.")
        exit(1)
    nodes_config = config_data["nodes"]
    print("Start to run the experiment!")
    f_latency_intra_progs_remove(xml_file_path, user_name, key_path, nodes_config)
    f_latency_intra_progs_run(xml_file_path, user_name, key_path, nodes_config)
    time = datetime.datetime.now()
    timestamp = time.strftime("%Y%m%d%H%M%S")
    file_name = f"latency_intra_{timestamp}.txt"
    local_file_path = os.path.join("results/latency/", file_name)
    f_latency_intra_result_pull(
        xml_file_path, user_name, key_path, nodes_config, local_file_path
    )
    f_latency_intra_progs_remove(xml_file_path, user_name, key_path, nodes_config)
    print(
        f"Have finished the experiment.\n"
        f"Due to large number, the results are in file results/latency/{file_name}."
    )


def f_latency_internet_run(xml_file_path, user_name, key_path, config_file_path):
    try:
        with open(config_file_path, "r") as file:
            config_data = json.load(file)
    except FileNotFoundError:
        print("Error: The file was not found, please configure first.")
        exit(1)
    nodes_config = config_data["nodes"]
    f_latency_intra_progs_remove(xml_file_path, user_name, key_path, nodes_config)
    f_latency_internet_progs_run(xml_file_path, user_name, key_path, nodes_config)
    client_index = nodes_config["client"][0]
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    client_ip = ssh_ips["node" + str(client_index)]
    cmd = f"python3 client/remote_client.py {client_ip}"
    os.system(cmd)
    time = datetime.datetime.now()
    timestamp = time.strftime("%Y%m%d%H%M%S")
    file_name = f"latency_internet_{timestamp}.txt"
    local_file_path = os.path.join("results/latency/", file_name)
    f_latency_intra_result_pull(
        xml_file_path, user_name, key_path, nodes_config, local_file_path
    )
    f_latency_intra_progs_remove(xml_file_path, user_name, key_path, nodes_config)


def f_latency_inter_run(xml_file_path, user_name, key_path, config_file_path):
    try:
        with open(config_file_path, "r") as file:
            config_data = json.load(file)
    except FileNotFoundError:
        print("Error: The file was not found, please configure first.")
        exit(1)
    nodes_config = config_data["nodes"]
    f_latency_intra_progs_remove(xml_file_path, user_name, key_path, nodes_config)
    f_latency_internet_progs_run(xml_file_path, user_name, key_path, nodes_config)
    client_index = nodes_config["client"][0]
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    client_ip = ssh_ips["node" + str(client_index)]
    cmds = [f"nohup python3 client/remote_client.py {client_ip} > /dev/null 2>&1 &"]
    external_ip = ssh_ips["external"]
    ssh_connection = f_ssh_connection_create(external_ip, user_name, key_path)
    f_cmds_exec(ssh_connection, cmds)
    ssh_connection.close()
    time = datetime.datetime.now()
    timestamp = time.strftime("%Y%m%d%H%M%S")
    file_name = f"latency_inter_{timestamp}.txt"
    local_file_path = os.path.join("results/latency/", file_name)
    f_latency_intra_result_pull(
        xml_file_path, user_name, key_path, nodes_config, local_file_path
    )
    f_latency_intra_progs_remove(xml_file_path, user_name, key_path, nodes_config)


def f_bot_beaver_run(xml_file_path, user_name, key_path, config_file_path, ratio):
    try:
        with open(config_file_path, "r") as file:
            config_data = json.load(file)
    except FileNotFoundError:
        print("Error: The file was not found, please configure first.")
        exit(1)

    nodes_config = config_data["nodes"]
    f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config)
    f_bot_programs_run(xml_file_path, user_name, key_path, nodes_config)
    time.sleep(5)
    f_bot_ss_deploy(xml_file_path, user_name, key_path, nodes_config)
    time.sleep(3)
    client_index = nodes_config["client"][0]
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    client_ip = ssh_ips["node" + str(client_index)]
    ssh_connection = f_ssh_connection_create(client_ip, user_name, key_path)
    cmds = [f"cd client && sudo ./main bot beaver {ratio}"]
    f_cmds_exec(ssh_connection, cmds)
    ssh_connection.close()
    time_name = datetime.datetime.now()
    timestamp = time_name.strftime("%Y%m%d%H%M%S")
    file_name = f"bot_beaver_{ratio}_{timestamp}.txt"
    local_file_path = os.path.join("results/bot/", file_name)
    f_bot_result_pull(xml_file_path, user_name, key_path, nodes_config, local_file_path)
    f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config)


def f_bot_poll_run(xml_file_path, user_name, key_path, config_file_path, ratio):
    try:
        with open(config_file_path, "r") as file:
            config_data = json.load(file)
    except FileNotFoundError:
        print("Error: The file was not found, please configure first.")
        exit(1)
    nodes_config = config_data["nodes"]
    f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config)
    f_bot_poll_programs_run(xml_file_path, user_name, key_path, nodes_config)
    client_index = nodes_config["client"][0]
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    client_ip = ssh_ips["node" + str(client_index)]
    ssh_connection = f_ssh_connection_create(client_ip, user_name, key_path)
    cmds = [f"cd client && sudo ./main bot poll {ratio}"]
    f_cmds_exec(ssh_connection, cmds)
    ssh_connection.close()
    time_name = datetime.datetime.now()
    timestamp = time_name.strftime("%Y%m%d%H%M%S")
    file_name = f"bot_poll_{ratio}_{timestamp}.txt"
    local_file_path = os.path.join("results/bot/", file_name)
    f_bot_result_pull(xml_file_path, user_name, key_path, nodes_config, local_file_path)
    f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config)


def f_bot_laiyang_run(xml_file_path, user_name, key_path, config_file_path, ratio):
    try:
        with open(config_file_path, "r") as file:
            config_data = json.load(file)
    except FileNotFoundError:
        print("Error: The file was not found, please configure first.")
        exit(1)
    nodes_config = config_data["nodes"]
    f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config)
    f_bot_laiyang_programs_run(xml_file_path, user_name, key_path, nodes_config)
    client_index = nodes_config["client"][0]
    ssh_ips = f_map_ssh_ip_get(xml_file_path)
    client_ip = ssh_ips["node" + str(client_index)]
    ssh_connection = f_ssh_connection_create(client_ip, user_name, key_path)
    cmds = [f"cd client && sudo ./main bot laiyang {ratio}"]
    f_cmds_exec(ssh_connection, cmds)
    ssh_connection.close()
    time_name = datetime.datetime.now()
    timestamp = time_name.strftime("%Y%m%d%H%M%S")
    file_name = f"bot_laiyang_{ratio}_{timestamp}.txt"
    local_file_path = os.path.join("results/bot/", file_name)
    f_bot_result_pull(xml_file_path, user_name, key_path, nodes_config, local_file_path)
    f_rate_progs_remove(xml_file_path, user_name, key_path, nodes_config)
