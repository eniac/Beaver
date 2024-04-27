from utils.config_task_gene import *
import json


def f_rate_config_gene(lb_num, config_file_path, active_nodes):
  nodes_config = f_nodes_config_gene(lb_num, active_nodes)
  backends_index = nodes_config['backend']
  clients_index = nodes_config['client']
  services, requests = f_rate_services_config_gene(backends_index, 
                                                   clients_index)
  configurations = {
    "nodes": nodes_config,
    "services": services,
    "requests": requests
  }
  with open(config_file_path, "w") as output:
    json.dump(configurations, output, indent=2)
  return nodes_config


def f_load_config_gene(lb_num, load_type, config_file_path, active_nodes):
  nodes_config = f_nodes_config_gene(lb_num, active_nodes)
  backends_index = nodes_config['backend']
  clients_index = nodes_config['client']
  services, requests = f_load_services_config_gene(backends_index, 
                                                   clients_index, load_type)
  configurations = {
    "nodes": nodes_config,
    "services": services,
    "requests": requests
  }
  with open(config_file_path, "w") as output:
    json.dump(configurations, output, indent=2)
  return nodes_config


def f_latency_intra_config_gene(config_file_path, active_nodes):
  nodes_config = f_nodes_config_gene(1, active_nodes)
  backends_index = nodes_config['backend']
  services, requests = f_latency_intra_services_gene(backends_index)
  configurations = {
    "nodes": nodes_config,
    "services": services,
    "requests": requests
  }
  with open(config_file_path, "w") as output:
    json.dump(configurations, output, indent=2)
  return nodes_config


def f_bot_config_gene(config_file_path, active_nodes):
  nodes_config = f_bot_nodes_config_gene(active_nodes)
  backends_index = nodes_config['backend']
  services, requests = f_bot_services_gene(backends_index)
  configurations = {
    "nodes": nodes_config,
    "services": services,
    "requests": requests
  }
  with open(config_file_path, "w") as output:
    json.dump(configurations, output, indent=2)
  return nodes_config