#include "service_self_define.h"
#include <cstring>
#include <fstream>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

std::vector<uint32_t> bot_record_vec;
uint32_t poll_ssid;
std::mutex poll_ssid_mutex;

void
f_service_latency_test()
{
    int sock_fd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1024];

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        std::cerr << "Error opening socket" << std::endl;
        return;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8000);

    if (bind(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        return;
    }
    std::cout << "Starting up on port 8000" << std::endl;
    while (true) {
        socklen_t len = sizeof(client_addr);
        int n = recvfrom(sock_fd, buffer, 1024, 0, (struct sockaddr*)&client_addr, &len);
        if (n > 0) {
            buffer[n] = '\0';
            sendto(sock_fd, buffer, n, 0, (struct sockaddr*)&client_addr, len);
        }
    }
}

static int
ringbuf_callback(void* ctx, void* data, size_t size)
{
    if (size == sizeof(uint32_t)) {
        uint32_t new_bot_data = *(uint32_t*)data;
        bot_record_vec.push_back(new_bot_data);
    }
    return 0;
}

void
f_service_bot_test()
{
    int sock_fd;
    struct sockaddr_in service_addr, client_addr;
    char buffer[4096];
    socklen_t client_len = sizeof(client_addr);
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&service_addr, 0, sizeof(service_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    service_addr.sin_family = AF_INET;
    service_addr.sin_addr.s_addr = INADDR_ANY;
    service_addr.sin_port = htons(8000);
    if (bind(sock_fd, (const struct sockaddr*)&service_addr, sizeof(service_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    int bot_record_fd = f_bpf_obj_get("/sys/fs/bpf/bot_record");
    if (bot_record_fd < 0) {
        printf("Error getting bot_record fd\n");
        exit(-1);
    }
    struct ring_buffer* rb = NULL;
    rb = ring_buffer__new(bot_record_fd, ringbuf_callback, NULL, NULL);
    if (!rb) {
        printf("Failed to create ring buffer\n");
        exit(-1);
    }
    printf("Server is running on port 8000\n");

    while (true) {
        int message_len = recvfrom(sock_fd, buffer, 4096, MSG_WAITALL, (struct sockaddr*)&client_addr, &client_len);
        if (message_len < 0) {
            perror("Error receiving message");
            continue;
        }
        buffer[message_len] = '\0';
        if (strcmp(buffer, "Request") == 0) {
            sendto(
                sock_fd,
                (const char*)buffer,
                strlen(buffer),
                MSG_CONFIRM,
                (const struct sockaddr*)&client_addr,
                client_len);
            int err = ring_buffer__poll(rb, 10);
            if (err < 0) {
                printf("Error polling ring buffer, %d\n", err);
                break;
            }
        } else if (strcmp(buffer, "finish") == 0) {
            for (auto& bot_data : bot_record_vec) {
                sendto(
                    sock_fd,
                    (const char*)&bot_data,
                    sizeof(bot_data),
                    MSG_CONFIRM,
                    (const struct sockaddr*)&client_addr,
                    client_len);
            }
            bot_record_vec.clear();
            const char* end_message = "end";
            sendto(
                sock_fd,
                end_message,
                strlen(end_message),
                MSG_CONFIRM,
                (const struct sockaddr*)&client_addr,
                client_len);
        } else if (strcmp(buffer, "start") == 0) {
            std::thread poll_thread(f_poll_snapshot);
            poll_thread.detach();
        } else if (strcmp(buffer, "RequestP") == 0) {
            sendto(
                sock_fd,
                (const char*)buffer,
                strlen(buffer),
                MSG_CONFIRM,
                (const struct sockaddr*)&client_addr,
                client_len);
            poll_ssid_mutex.lock();
            bot_record_vec.push_back(poll_ssid);
            poll_ssid_mutex.unlock();
        }
    }
}

void
f_service_bot_ly_test(char* ip)
{
    int sock_fd;
    struct sockaddr_in service_addr, client_addr;
    char buffer[4096];
    socklen_t client_len = sizeof(client_addr);
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (strcmp(ip, "service") == 0) {
        std::thread ly_service_thread(f_ly_service_logic);
        ly_service_thread.detach();
    } else {
        std::thread ly_client_thread(f_ly_client_logic, ip);
        ly_client_thread.detach();
    }

    memset(&service_addr, 0, sizeof(service_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    service_addr.sin_family = AF_INET;
    service_addr.sin_addr.s_addr = INADDR_ANY;
    service_addr.sin_port = htons(8000);
    if (bind(sock_fd, (const struct sockaddr*)&service_addr, sizeof(service_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    int bot_record_fd = f_bpf_obj_get("/sys/fs/bpf/bot_record");
    if (bot_record_fd < 0) {
        printf("Error getting bot_record fd\n");
        exit(-1);
    }
    struct ring_buffer* rb = NULL;
    rb = ring_buffer__new(bot_record_fd, ringbuf_callback, NULL, NULL);
    if (!rb) {
        printf("Failed to create ring buffer\n");
        exit(-1);
    }
    printf("Server is running on port 8000\n");

    while (true) {
        int message_len = recvfrom(sock_fd, buffer, 4096, MSG_WAITALL, (struct sockaddr*)&client_addr, &client_len);
        if (message_len < 0) {
            perror("Error receiving message");
            continue;
        }
        buffer[message_len] = '\0';
        if (strcmp(buffer, "Request") == 0) {
            sendto(
                sock_fd,
                (const char*)buffer,
                strlen(buffer),
                MSG_CONFIRM,
                (const struct sockaddr*)&client_addr,
                client_len);
            int err = ring_buffer__poll(rb, 10);
            if (err < 0) {
                printf("Error polling ring buffer, %d\n", err);
                break;
            }
        } else if (strcmp(buffer, "finish") == 0) {
            for (auto& bot_data : bot_record_vec) {
                sendto(
                    sock_fd,
                    (const char*)&bot_data,
                    sizeof(bot_data),
                    MSG_CONFIRM,
                    (const struct sockaddr*)&client_addr,
                    client_len);
            }
            bot_record_vec.clear();
            const char* end_message = "end";
            sendto(
                sock_fd,
                end_message,
                strlen(end_message),
                MSG_CONFIRM,
                (const struct sockaddr*)&client_addr,
                client_len);
        } else if (strcmp(buffer, "start") == 0) {
            std::thread poll_thread(f_poll_snapshot);
            poll_thread.detach();
        } else if (strcmp(buffer, "RequestP") == 0) {
            sendto(
                sock_fd,
                (const char*)buffer,
                strlen(buffer),
                MSG_CONFIRM,
                (const struct sockaddr*)&client_addr,
                client_len);
            poll_ssid_mutex.lock();
            bot_record_vec.push_back(poll_ssid);
            poll_ssid_mutex.unlock();
        }
    }
}

void
f_poll_snapshot()
{
    poll_ssid = 0;
    while (true) {
        usleep(200);
        poll_ssid_mutex.lock();
        poll_ssid++;
        poll_ssid_mutex.unlock();
    }
}

void
f_ly_service_logic()
{
    int sock_fd, conn_fd;
    struct sockaddr_in service_addr, client_addr;
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("Socket creation failed");
        exit(0);
    }
    bzero(&service_addr, sizeof(service_addr));
    service_addr.sin_family = AF_INET;
    service_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    service_addr.sin_port = htons(7000);
    if ((bind(sock_fd, (struct sockaddr*)&service_addr, sizeof(service_addr))) != 0) {
        perror("Socket bind failed");
        exit(0);
    }
    if ((listen(sock_fd, 5)) != 0) {
        perror("Listen failed");
        exit(0);
    }
    socklen_t len = sizeof(client_addr);
    conn_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &len);
    if (conn_fd < 0) {
        perror("Server accept failed");
        exit(0);
    }
    uint32_t buffer;
    while (1) {
        uint32_t n = read(conn_fd, &buffer, sizeof(buffer));
        if (n > 0) {
            uint32_t new_ssid = ntohl(buffer);
            poll_ssid_mutex.lock();
            poll_ssid = new_ssid;
            poll_ssid_mutex.unlock();
        }
    }
}

void
f_ly_client_logic(char* ip)
{
    int sock_fd;
    struct sockaddr_in service_addr;
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("Socket creation failed");
        exit(0);
    }
    int opt = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt(SO_REUSEADDR) failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_port = htons(7000);
    if (bind(sock_fd, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        perror("Bind failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    service_addr.sin_family = AF_INET;
    service_addr.sin_addr.s_addr = inet_addr(ip);
    service_addr.sin_port = htons(7000);
    int connect_result, attempt_count = 0;
    while (attempt_count < 5) {
        connect_result = connect(sock_fd, (struct sockaddr*)&service_addr, sizeof(service_addr));
        if (connect_result == 0) {
            printf("Successfully connected to the server.\n");
            break;
        } else {
            perror("Connection with the server failed");
            attempt_count++;
            sleep(3);
        }
    }
    poll_ssid = 0;
    uint32_t buffer;
    unsigned long next_write_time = 0;
    unsigned long time_passed = 0;
    srand(time(NULL));
    next_write_time = rand() % 5001;
    while (1) {
        usleep(200);
        poll_ssid_mutex.lock();
        poll_ssid++;
        buffer = htonl(poll_ssid);
        poll_ssid_mutex.unlock();
        time_passed += 200;
        if (time_passed >= next_write_time) {
            write(sock_fd, &buffer, sizeof(buffer));
            time_passed = 0;
            next_write_time = rand() % 5001;
        }
    }
    close(sock_fd);
}