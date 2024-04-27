# Files
src_files := main.cc $(wildcard src/*.cc) $(wildcard src/*.h)
launch_ss_files := launch_ss.cc

# Output files
main_exe = main
launch_ss_exe = launch_ss

pkgconf ?= pkg-config

# Build using pkg-config variables if possible
ifneq ($(shell $(pkgconf) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

pc_file := $(shell $(pkgconf) --path libdpdk 2>/dev/null)
cflags += -O3 $(shell $(pkgconf) --cflags libdpdk)
ldflags = $(shell $(pkgconf) --libs libdpdk)

cflags += -DALLOW_EXPERIMENTAL_API

# Targets
all: $(main_exe) $(launch_ss_exe)

$(main_exe): $(src_files) Makefile $(pc_file)
	g++ $(cflags) $(src_files) -o $@ $(LDFLAGS) $(ldflags) -ljsoncpp

$(launch_ss_exe): $(launch_ss_files)
	g++ -O2 $(launch_ss_files) -o $@

.PHONY: clean
clean:
	rm -f $(main_exe) $(launch_ss_exe)
