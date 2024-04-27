#ifndef CLIENT_SELF_DEFINE_H_
#define CLIENT_SELF_DEFINE_H_

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

void
f_latency_test_client(const std::string& service_ip, int service_port);

void
f_latency_test_tcp_server(int port);

void
f_udp_message_send(const char* message);

void
f_bot_beaver_test(float bot_ratio);

void
f_bot_poll_test(float bot_ratio);

void
f_bot_laiyang_test(float bot_ratio);

void
f_bot_udp_req(int sock_fd, struct sockaddr_in service_addr, const char* message);

void
f_bot_results_collect(int sock_fd, struct sockaddr_in service_addr, std::vector<uint32_t>& results);

#endif // CLIENT_SELF_DEFINE_H_