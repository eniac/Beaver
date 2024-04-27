# Files
src_files := main.cc $(wildcard src/*.cc) $(wildcard src/*.h)

# Output file
main_exe = main

pkgconf ?= pkg-config

# Build using pkg-config variables if possible
ifneq ($(shell $(pkgconf) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

pc_file := $(shell $(pkgconf) --path libdpdk 2>/dev/null)
cflags += -O3 $(shell $(pkgconf) --cflags libdpdk)
ldflags = $(shell $(pkgconf) --libs libdpdk)

cflags += -DALLOW_EXPERIMENTAL_API

all: $(main_exe)

$(main_exe): $(src_files) Makefile $(pc_file)
	g++ $(cflags) $(src_files) -o $@ $(LDFLAGS) $(ldflags) -ljsoncpp

.PHONY: clean
clean:
	rm -f $(main_exe)
