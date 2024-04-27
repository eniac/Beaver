#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <getopt.h>

using namespace std;

int main(int argc, char *argv[]) {
  const char* fifo_path = "/tmp/myfifo";
  int ss_signal = 1;
  int fifo_fd = open(fifo_path, O_WRONLY);
  if (fifo_fd == -1) {
    cout << "Failed to open FIFO\n";
    return 1;
  }

  int frequency = -1;
  int count = -1;
  int block = -1;

  int opt;
  while ((opt = getopt(argc, argv, "f:c:b:")) != -1) {
    switch (opt) {
      case 'f':
        frequency = static_cast<int>(strtoul(optarg, nullptr, 10));
        break;
      case 'c':
        count = static_cast<int>(strtoul(optarg, nullptr, 10));
        break;
      case 'b':
        block = static_cast<int>(strtoul(optarg, nullptr, 10));
        break;
      default: /* '?' */
        cerr << "Usage: " << argv[0] << " -f frequency -c count -b block\n";
        exit(EXIT_FAILURE);
    }
  }

  if (frequency == -1 || count == -1 || block == -1) {
    cerr << "Invalid arguments.\n";
    cerr << "Usage: " << argv[0] << " -f frequency -c count -b block\n";
    exit(EXIT_FAILURE);
  }
  struct Data {
    int frequency;
    int count;
    int block;
  } data;
  data.frequency = frequency;
  data.count = count;
  data.block = block;
  if (write(fifo_fd, &data, sizeof(data)) != sizeof(data)) 
    cout << "Failed to write data\n";
  close(fifo_fd);
  return 0;
}
