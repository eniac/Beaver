# Beaver

Beaver is a practical partial snapshot protocol that guarantees causal consistency under external traffic interference.
By exploiting the placement of software load balancers in data center networks and their associated communication pattern, Beaver not only requires minimal changes to today’s data center operations, but also eliminates any form of blocking to existing distributed communication and its related impact to user traffic.

## Repo Structure

The repo contains the prototype implementation of Beaver tested extensively on CloudLab.
It also packages the automated scripts for the ease of setting up the environment and reproducing the experiments on CloudLab.

* `beaver.py`: The entry point to set up the experiment environment, run the experiment, and collect the results---all from the local desktop.
* `cloudlab/`: The default folder containing (a) a dummy manifest file with contents to be replaced with the actual manifest file from the CloudLab portal and (b) the Beaver profile for the ease of instantiating CloudLab experiments.
* `install_env.sh`: The script to install the required dependencies on the machines.
* `backend/`: The backend server implementation, including the XDP program encoding typical SLB-related processing (re-computation of
checksums, NAT caching in a shared eBPF map, and the de-encapsulation of incoming packets) and Beaver's in-group process logic. It also includes the Linux tc to look up the NAT entries and perform the header transformations to replicate Direct Server Return (DSR).
* `client/`: A sample client implementation.
* `controller/`: Beaver's controller server that initiates, controls, collects the results of the snapshots. It also applies the detection algorithm for verifying the snapshots.
* `load_balancer`: The software load balancer implementation that emulates the typical behvaiors using DPDK, including consistent hashing, caching, SLB-forwarding based on Virtual IP (VIP), and encapsulation. It also includes the optimistic gateway marking logic for Beaver.
* `uilts/`: The utility scripts for automating the experiments.

## Resource Requirements

Beaver has been tested extensively on CloudLab, and the artifact automates the process as much as possible for the ease of reproducing the experiments all on CloudLab.

* To include ECMP forwarding behavior in the experiment setup, the artifact requires reserving 1 dell-s4048 switch from CloudLab.

* Due to CloudLab's [wiring constraints](https://docs.cloudlab.us/hardware.html) with dell-s4048 switches, the artifact requires xl170 machines for instantiating global controller, software load balancers, backend servers, and so on.

* `cloudlab/beaver_profile.py` contains the profile for the ease of instantiating the experiment on Cloudlab, which includes 1 dell-s4048 switch, 1 `c220g2` node for the external client, and a user-specified number of `xl170` nodes.
A **minimum specification of 6 xl170 machines** is required to execute the experiments, in addition to the switch and a `c220g2` node (which the profile instantiates by default).
The maximum scale experiment requires a minimum reservation of 34 xl170 machines.

**Notes for resource reservation**

* CloudLab has around 5 dell-s4048 switches available which may imply contending resource reservation among multiple reviewers. Similar contention may occur for xl170 reservation especially for large-scale experiments.
* As the `dell-s4048` switch feature is relatively new, failed direct connectivity between xl170 node and the switch can occur. Thus, it is recommended to reserve more xl170 nodes than the minimum requirement (especially for a larger scale experiments). `beaver.py` will certify the working connectivity and give a warning if the effective number of xl170 nodes is less than required for the experiment.

## Experiment Setup with CloudLab / Kick-the-tires Instructions

We provide set-by-step instructions for reserving Beaver's resources on CloudLab and setting up the environment.
These steps need to be followed for each new CloudLab reservation.

0. Clone the Beaver repository and navigate to the root directory. Install the python3 dependencies for automated experiment management on remote machines from the local desktop: `pip3 install -r requirements.txt`.

1. Instantiate an experiment on CloudLab using the profile `cloudlab/beaver_profile.py` described above.

2. Specify a minimum of 6 xl170 nodes in the profile. It is recommended to reserve more xl170 nodes than the target to account for potential faulty phy connectivity between xl170 nodes and the `dell-s4048` switch (see resource requirements above).

![cloudlab_reservation_parameter.png](img/cloudlab_reservation_parameter.png)

3. Move on to finalize the experiment reservation.

![cloudlab_reservation_topo.png](img/cloudlab_reservation_topo.png)

4. Once the experiments are ready, copy the XML contents under the `manifest` column (right next to the `List View` column) on the experiment profile page, and replace the contents in `cloudlab/manifest.xml` file.

5. Install dependencies on all machines in parallel by using `python3 beaver.py env` subcommand with the CloudLab user name (`--user_name`/`-u`) and the SSH private key file path (`--ssh_key` / `-k`).
   
* Example command: `python3 beaver.py -u leoyu -k ~/.ssh/leoyu env`.
* By default, it will reads the manifest file of path `cloudlab/manifest.xml` to extract the information for the CloudLab machines. Otherwise, specify the path to the manifest file using the `--manifest` / `-m` option.
* The whole process takes around 20 minutes to complete. The terminal will print `Finish configuring the environment for all the nodes!` to indicate successful installation.
