#!/bin/bash

sudo apt-get -y update

# Install essential tools and libraries
sudo apt install -y build-essential libnuma-dev python3-pip pkg-config linuxptp
# Libs for RDMA and logging
sudo apt install -y librdmacm-dev libibverbs-dev rsyslog

# Install and configure for DPDK
sudo apt install -y dpdk dpdk-dev
# Configure hugepages for DPDK 
echo 'vm.nr_hugepages=2048' | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
cat /proc/meminfo | grep Huge
# Create and mount a directory for hugepages
sudo mkdir /mnt/huge
sudo mount -t hugetlbfs nodev /mnt/huge
echo "nodev /mnt/huge hugetlbfs defaults 0 0" | sudo tee -a /etc/fstab
# Update the shared library cache to include new libraries
sudo /sbin/ldconfig -v

# Install additional tools and libraries needed for packet processing and performance analysis
sudo apt-get -y install clang llvm libelf-dev gcc make iproute2 bpfcc-tools linux-headers-$(uname -r)
sudo apt-get -y install linux-tools-common libbpf-dev linux-tools-generic iperf
# Create symbolic links for system include directories
sudo ln -s /usr/include/x86_64-linux-gnu/asm /usr/include/asm
# Mount bpf filesystem for BPF tools
sudo mount -t bpf none /sys/fs/bpf/
echo "none /sys/fs/bpf bpf defaults 0 0" | sudo tee -a /etc/fstab
# Increase BPF JIT limit for high performance BPF applications
sudo sysctl -w net.core.bpf_jit_limit=30000000

# Allow UDP traffic on port 514 for remote syslog
sudo ufw allow 514/udp
# Install JSON library for C++ and create a symbolic link for include path
sudo apt-get install -y libjsoncpp-dev
sudo ln -s /usr/include/jsoncpp/json/ /usr/include/json
sudo ip route add 192.168.100.0/24 dev eno50np1
