import xml.etree.ElementTree as ET

NAMESPACES = {"default": "http://www.geni.net/resources/rspec/3"}


def f_xml_info_get(xml_file_path):
    """Read an xml file and return the root element."""
    tree = ET.parse(xml_file_path)
    xml_info = tree.getroot()
    return xml_info


def f_map_ssh_ip_get(xml_file_path):
    """Extract the ips using for ssh connection."""
    xml_info = f_xml_info_get(xml_file_path)
    ssh_ips = {}
    for host in xml_info.findall(".//default:host", NAMESPACES):
        node_name = host.get("name").split(".")[0]
        ssh_ip = host.get("ipv4")
        if "node" in node_name:
            ssh_ips[node_name] = ssh_ip
        if "external" in node_name:
            ssh_ips[node_name] = ssh_ip
    return ssh_ips


def f_maps_local_ip_get(xml_file_path):
    """
    Extract the map between nodes and their local ips.
    (Including data plane ip, control plane ip, gateway ip of switch port.)
    """
    xml_info = f_xml_info_get(xml_file_path)
    interfaces_info = xml_info.findall(".//default:interface", NAMESPACES)
    ips_info = xml_info.findall(".//default:ip", NAMESPACES)
    node_names = []
    ips = []
    for interface_info in interfaces_info:
        node_name = interface_info.get("client_id").split(":")[0]
        if "node" in node_name:
            node_names.append(node_name.split(":")[0])
    for ip_info in ips_info:
        ip = ip_info.get("address")
        ips.append(ip)
    data_plane_ips = {}
    ctrl_plane_ips = {}
    for i in range(len(ips)):
        if "192.168.255" in ips[i]:
            ctrl_plane_ips[node_names[i]] = ips[i]
        else:
            if "192.168" in ips[i]:
                data_plane_ips[node_names[i]] = ips[i]
    sw_gateway_ips = {}
    for node_name, ip in data_plane_ips.items():
        sw_gateway_ips[node_name] = ip[:-1] + "1"
    return ctrl_plane_ips, data_plane_ips, sw_gateway_ips


def f_map_sw_port_get(xml_file_path):
    """Extract the map between nodes and switch ports."""
    xml_info = f_xml_info_get(xml_file_path)
    interface_refs = xml_info.findall(".//default:interface", NAMESPACES)
    node_names = []
    sw_interface_names = []
    for interface_ref in interface_refs:
        interface_name = interface_ref.get("component_id").split("%")[-1]
        if "eth1" in interface_name:
            continue
        interface_info = interface_ref.get("client_id")
        node_name = interface_info.split(":")[0]
        if "node" not in node_name:
            interface_name = interface_name.split("F")[-1]
            sw_interface_names.append(interface_name)
        else:
            node_names.append(node_name)
    sw_ports = {}
    for i in range(len(node_names)):
        sw_ports[node_names[i]] = sw_interface_names[i]
    return sw_ports
