# Beaver

Beaver is a practical partial snapshot protocol that guarantees causal consistency under external traffic interference.
By exploiting the placement of software load balancers in data center networks and their associated communication pattern, Beaver not only requires minimal changes to today’s data center operations, but also eliminates any form of blocking to existing distributed communication and its related impact to user traffic.

## Repo Structure

The repo contains the prototype implementation of Beaver tested extensively on Cloudlab.
It also packages the automated scripts for the ease of setting up the environment and reproducing the experiments on Cloudlab.

* `beaver.py`: The entry point to set up the experiment environment, run the experiment, and collect the results.
* `backend/`: The backend server implementation, including the XDP program encoding typical SLB-related processing (re-computation of
checksums, NAT caching in a shared eBPF map, and the de-encapsulation of incoming packets) and Beaver's in-group process logic. It also includes the Linux tc to look up the NAT entries and perform the header transformations to replicate Direct Server Return (DSR).
* `client/`: A sample client implementation.
* `cloudlab_profiles/`: The Cloudlab profile for instantiating the experiment.
* `controller/`: Beaver's controller server that initiates, controls, collects the results of the snapshots. It also applies the detection algorithm for verifying the snapshots.
* `load_balancer`: The software load balancer implementation that emulates the typical behvaiors using DPDK, including consistent hashing, caching, SLB-forwarding based on Virtual IP (VIP), and encapsulation. It also includes the optimistic gateway marking logic for Beaver.
* `manifest_files`: A dummy manifest file with contents to be replaced with the actual manifest file from the Cloudlab portal.
* `uilts/`: The utility scripts for automating the experiments.
* `install_env.sh`: The script to install the required dependencies on the machines.

## Resource Requirements

Beaver has been tested extensively on Cloudlab, and the artifact automates the process as much as possible for the ease of reproducing the experiments all on Cloudlab.

* To include ECMP forwarding behavior in the experiment setup, the artifact requires reserving 1 dell-s4048 switch from Cloudlab.

* Due to Cloudlab's resource coupling with dell-s4048 switch, the artifact requires xl170 machines for instantiating global controller, software load balancers, backend servers, and so on.

* `cloudlab_profiles/beaver_profile.py` contains the profile for instantiating the experiment on Cloudlab, which includes 1 dell-s4048 switch, 1 `c220g2` node for the external client, and a user-specified number of `xl170` nodes.
A **minimum of 6 xl170 machines** is required to execute the experiments, in addition to the switch and `c220g2` node.
The maximum scale experiment requires a **minimum reservation of 33 xl170 machines**.

**Notes for resource reservation**

* Cloudlab has around 5 dell-s4048 switches available which may imply contending resource reservation among multiple reviewers. Similar contention may occur for xl170 reservation especially for large-scale experiments.
* As the `dell-s4048` switch feature is relatively new, failed direct connectivity between xl170 node and the switch can occur. Thus, it is recommended to reserve 2-3 more xl170 nodes than target. `beaver.py` will certify the working connectivity and give a warning if the effective number of xl170 nodes is less than required for the experiment.
