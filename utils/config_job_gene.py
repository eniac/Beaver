

def f_rate_service_config_gene(ip, port, virt_ip, virt_port, protocol, 
                               node_name):
  return {
    "application": 'iperf',
    "virt_ip": virt_ip,
    "virt_port": virt_port,
    "ip": ip,
    "port": port,
    "protocol": protocol,
    "node_name": node_name
  }


def f_latency_service_config_gene(ip, port, virt_ip, virt_port, protocol, 
                                  node_name):
  return {
    "application": 'latency',
    "virt_ip": virt_ip,
    "virt_port": virt_port,
    "ip": ip,
    "port": port,
    "protocol": protocol,
    "node_name": node_name
  }


def f_bot_service_config_gene(ip, port, virt_ip, virt_port, protocol, 
                              node_name):
  return {
    "application": 'bot',
    "virt_ip": virt_ip,
    "virt_port": virt_port,
    "ip": ip,
    "port": port,
    "protocol": protocol,
    "node_name": node_name
  }


def f_rate_request_config_gene(ip, virt_ip, virt_port, protocol, node_name):
  return {
    "application": 'iperf',
    "virt_ip": virt_ip,
    "virt_port": virt_port,
    "ip": ip,
    "protocol": protocol,
    "mtu": 100,
    "bandwidth": '30M',
    "node_name": node_name
  }


def f_load_cassandra_config_gene(ip, port, virt_ip, virt_port,
                                 backends_index, node_name):
  return {
    "application": 'cassandra',
    "virt_ip": virt_ip,
    "virt_port": virt_port,
    "ip": ip,
    "protocol": "tcp",
    "port": port,
    "group": backends_index,
    "node_name": node_name
  }


def f_load_cassandra_request_gene(virt_ip, virt_port, node_name):
  return {
    "application": 'cassandra',
    "virt_ip": virt_ip,
    "virt_port": virt_port,
    "node_name": node_name
  }


def f_load_iperf_request_config_gene(ip, virt_ip, virt_port, protocol, 
                                     node_name):
  return {
    "application": 'iperf',
    "virt_ip": virt_ip,
    "virt_port": virt_port,
    "ip": ip,
    "protocol": protocol,
    "mtu": 800,
    "bandwidth": '1000M',
    "node_name": node_name
  }