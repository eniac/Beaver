def f_rate_service_deploy_cmd(service):
    cmds = []
    ip = service["ip"]
    port = service["port"]
    cmds.append(f"nohup sudo stdbuf -o0 iperf -s -u -p {port} -B {ip} 2>&1 &")
    # cmds.append(f"nohup sudo stdbuf -o0 iperf -s -p {port} -B {ip} 2>&1 &")
    return cmds


def f_rate_request_deploy_cmd(request):
    cmds = []
    ip = request["ip"]
    virt_ip = request["virt_ip"]
    virt_port = request["virt_port"]
    mtu = request["mtu"]
    bandwidth = request["bandwidth"]
    cmds.append(
        f"nohup sudo stdbuf -o0 iperf -c {virt_ip} -p {virt_port} "
        f"-t 0 -l {mtu} -B {ip} -b {bandwidth} -u 2>&1 &"
    )
    return cmds


def f_load_iperf_request_deploy_cmd(request, thread_num):
    cmds = []
    ip = request["ip"]
    virt_ip = request["virt_ip"]
    virt_port = request["virt_port"]
    mtu = request["mtu"]
    bandwidth = request["bandwidth"]
    cmds.append(
        f"nohup sudo stdbuf -o0 iperf -c {virt_ip} -p {virt_port} "
        f"-t 30 -l {mtu} -B {ip} -b {bandwidth} -P {thread_num} -u "
        f"> iperf_output.txt 2>&1 &"
    )
    return cmds


def f_cassandra_deploy_cmd(service):
    cmds = []
    ip = service["ip"]
    port = service["port"]
    backends_index = service["group"]
    backends_ip = [f"192.168.{index}.2:7000" for index in backends_index]
    seeds = ",".join(backends_ip)
    inner_group_cmd = (
        'sudo sed -i \'/^[[:space:]]*- seeds:/ s/"[^"]*"/"{}"/\' '
        "/etc/cassandra/cassandra.yaml".format(seeds)
    )
    cmds.append(inner_group_cmd)
    listen_address_cmd = (
        "sudo sed -i 's/^\( *\)listen_address:.*/\\1listen_address: {}/' "
        "/etc/cassandra/cassandra.yaml".format(ip)
    )
    cmds.append(listen_address_cmd)
    rpc_address_cmd = (
        "sudo sed -i 's/^\( *\)rpc_address:.*/\\1rpc_address: {}/' "
        "/etc/cassandra/cassandra.yaml".format(ip)
    )
    cmds.append(rpc_address_cmd)
    port_cmd = (
        "sudo sed -i 's/^\( *\)native_transport_port:.*/\\1native_transport_port"
        ": {}/' /etc/cassandra/cassandra.yaml".format(port)
    )
    cmds.append(port_cmd)
    cmds.append("sudo systemctl start cassandra")
    cmds.append("sudo systemctl restart cassandra")
    return cmds
