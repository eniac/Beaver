import random
import socket
import sys
import time


def f_remote_reqs(remote_ip):
    port = 20000
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((remote_ip, port))
        request_counter = 0
        print("Connected to the server")
        while request_counter < 100:
            for i in range(2):
                s.sendall(b"Hello, server")
                data = s.recv(1024)
            request_counter += 1
            if random.random() > 0.65:
                base_sleep_time = random.uniform(0.0001, 0.01)  # 10us-10ms
                time.sleep(base_sleep_time)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python client.py remote_ip")
        sys.exit(1)
    f_remote_reqs(sys.argv[1])
