"""This profile allocates two bare metal nodes and connects them together via a Dell or Mellanox switch with layer1 links. 

Instructions:
Click on any node in the topology and choose the `shell` menu item. When your shell window appears, use `ping` to test the link.

You will be able to ping the other node through the switch fabric. We have installed a minimal configuration on your
switches that enables the ports that are in use, and turns on spanning-tree (RSTP) in case you inadvertently created a loop with your topology. All
unused ports are disabled. The ports are in Vlan 1, which effectively gives a single broadcast domain. If you want anything fancier, you will need
to open up a shell window to your switches and configure them yourself.

If your topology has more then a single switch, and you have links between your switches, we will enable those ports too, but we do not put them into
switchport mode or bond them into a single channel, you will need to do that yourself.

If you make any changes to the switch configuration, be sure to write those changes to memory. We will wipe the switches clean and restore a default
configuration when your experiment ends."""

# Import the Portal object.
import geni.portal as portal
# Import the ProtoGENI library.
import geni.rspec.pg as pg
# Import the Emulab specific extensions.
import geni.rspec.emulab as emulab

# Create a portal context.
pc = portal.Context()

# Create a Request object to start building the RSpec.
request = pc.makeRequestRSpec()
pc.defineParameter("N", "Number of Nodes", portal.ParameterType.INTEGER, 5)
pc.defineParameter("type", "Type of Nodes", portal.ParameterType.STRING, "xl170")
pc.defineParameter("phystype", "Switch type", portal.ParameterType.STRING, "dell-s4048",
                   [('mlnx-sn2410', 'Mellanox SN2410'),('dell-s4048',  'Dell S4048')])
# Retrieve the values the user specifies during instantiation.
params = pc.bindParameters()
if params.N < 2:
    pc.reportError(portal.ParameterError("You must choose at least 2 nodes"))

# Count for node name.

# ifaces for data plane
ifaces_data_plane = []
ifaces_ctrl_plane = []
for i in range(params.N):
    node = request.RawPC("node" + str(i + 1))
    node.hardware_type = params.type
    # Create iface and assign IP
    iface = node.addInterface("eth1")
    iface.addAddress(pg.IPv4Address("192.168.255." + str(i + 1), "255.255.255.0"))
    ifaces_ctrl_plane.append(iface)
    iface = node.addInterface("eth2")
    iface.addAddress(pg.IPv4Address("192.168." + str(i + 1) + ".2", "255.255.255.0"))
    ifaces_data_plane.append(iface)
    
node = request.RawPC("external")
node.hardware_type = "c220g2"

# Now add the link to the rspec. 

all_swiface = []
mysw = request.Switch("mysw");
mysw.hardware_type = params.phystype
for i in range(params.N):
    swiface = mysw.addInterface()
    all_swiface.append(swiface)

lan_ctrl_plane = request.LAN("lan_ctrl_plane")
for iface in ifaces_ctrl_plane:
    lan_ctrl_plane.addInterface(iface)

all_link = []
for i in range(params.N):
    link = request.L1Link("link"+str(i+1))
    link.addInterface(ifaces_data_plane[i])
    link.addInterface(all_swiface[i])
    all_link.append(link)

# Print the RSpec to the enclosing page.
pc.printRequestRSpec(request)