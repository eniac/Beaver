# Directories
ebpf_kern_dir := src/kernel
ebpf_user_dir := src/user

# Files
kern_egress := $(ebpf_kern_dir)/lb_redirect.c
kern_ingress := $(ebpf_kern_dir)/client_snat.c
kern_header := $(ebpf_kern_dir)/lb_redirect.h
user_files := $(wildcard $(ebpf_user_dir)/*.cc) $(wildcard $(ebpf_user_dir)/*.h)
main_file := main.cc

# Output
kern_egress_obj := $(ebpf_kern_dir)/lb_redirect.o
kern_ingress_obj := $(ebpf_kern_dir)/client_snat.o
main_exe := main

# Targets
.PHONY: all load unload clean

all: $(kern_egress_obj) $(kern_ingress_obj) $(main_exe)

load: $(kern_egress_obj)  $(kern_ingress_obj) $(main_exe)
	sudo ./$(main_exe) load

unload: $(main_exe)
	sudo ./$(main_exe) unload

user: $(main_exe)
	sudo ./$(main_exe) user

intra: $(main_exe)
	sudo ./$(main_exe) intra

clean:
	rm -f $(kern_egress_obj) $(kern_ingress_obj) $(main_exe)

$(kern_egress_obj): $(kern_egress) $(kern_header)
	clang -O2 -target bpf -c $(kern_egress) -o $(kern_egress_obj)

$(kern_ingress_obj): $(kern_ingress) $(kern_header)
	clang -O2 -target bpf -c $(kern_ingress) -o $(kern_ingress_obj)

# Compile main.cc
$(main_exe): $(main_file) $(user_files)
	g++ -std=c++17 $(main_file) $(user_files) -o $(main_exe) -lbpf