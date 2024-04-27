#ifndef SERVICE_SELF_DEFINE_H_
#define SERVICE_SELF_DEFINE_H_

#include <iostream>
#include <cstring>
#include <vector>
#include <mutex>
#include <fstream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "bpf_prog_manage.h"


void f_service_latency_test();

void f_service_bot_test();

void f_service_bot_ly_test(char *ip);

void f_poll_snapshot();

void f_ly_service_logic();

void f_ly_client_logic(char *ip);

static int ringbuf_callback(void *ctx, void *data, size_t size);

#endif // SERVICE_SELF_DEFINE_H_