#include "client_self_define.h"
#include <unistd.h>

void
f_latency_test_client(const std::string& service_ip, int service_port)
{
    int local_port = 9000;
    std::string message = "Hello, server!";
    char buffer[1024];
    struct sockaddr_in service_addr, client_addr;
    socklen_t len = sizeof(service_addr);
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_port = htons(local_port);
    if (bind(sock_fd, (const struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        perror("Bind failed");
        close(sock_fd);
        exit(1);
    }

    memset(&service_addr, 0, sizeof(service_addr));
    service_addr.sin_family = AF_INET;
    service_addr.sin_port = htons(service_port);
    service_addr.sin_addr.s_addr = inet_addr(service_ip.c_str());
    std::random_device rd;
    std::mt19937 gen(rd());
    std::poisson_distribution<> d(3);
    int request_counter = 0;
    while (request_counter < 100) { // TODO(lc): Make the number of requests configurable.
        for (int i = 0; i < 2; i++) {
            sendto(
                sock_fd,
                message.c_str(),
                message.length(),
                MSG_CONFIRM,
                (const struct sockaddr*)&service_addr,
                sizeof(service_addr));
            recvfrom(sock_fd, buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr*)&service_addr, &len);
        }
        request_counter++;
        // int poisson_value = d(gen);
        // if (poisson_value > 3) {
        int baseSleepTime = rand() % 500 + 100;
        // int sleepTime = (poisson_value - 3) * baseSleepTime;
        // usleep(sleepTime);
        usleep(baseSleepTime);
        // }
    }
    close(sock_fd);
}

void
f_latency_test_tcp_server(int port)
{
    int server_fd, new_sock;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("TCP socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("TCP bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("TCP listen");
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d\n", port);
    while (1) {
        new_sock = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_sock < 0) {
            perror("TCP accept");
            exit(EXIT_FAILURE);
        }
        printf("TCP Connection Established.\n");
        while (1) {
            ssize_t bytes_read = read(new_sock, buffer, 1024);
            if (bytes_read <= 0) {
                break;
            }
            buffer[bytes_read] = '\0';
            f_udp_message_send(buffer);
            write(new_sock, buffer, strlen(buffer));
        }
        close(new_sock);
        printf("TCP Connection Closed.\n");
    }
}

void
f_udp_message_send(const char* message)
{
    int sock_fd;
    struct sockaddr_in service_addr, client_addr;
    char buffer[1024];
    socklen_t len;
    ssize_t n;
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("UDP socket creation failed");
        return;
    }
    memset(&service_addr, 0, sizeof(service_addr));
    service_addr.sin_family = AF_INET;
    service_addr.sin_port = htons(10000);
    service_addr.sin_addr.s_addr = inet_addr("192.168.100.1");

    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(9000);
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock_fd, (const struct sockaddr*)&client_addr, sizeof(client_addr));
    sendto(sock_fd, message, strlen(message), 0, (const struct sockaddr*)&service_addr, sizeof(service_addr));
    len = sizeof(service_addr);

    n = recvfrom(sock_fd, buffer, 1024, 0, (struct sockaddr*)&service_addr, &len);
    if (n > 0) {
        buffer[n] = '\0';
    } else {
        printf("No UDP response\n");
    }
    close(sock_fd);
}

void
f_bot_udp_req(int sock_fd, struct sockaddr_in service_addr, const char* message)
{
    if (sendto(sock_fd, message, strlen(message), 0, (struct sockaddr*)&service_addr, sizeof(service_addr)) < 0) {
        perror("Sendto failed");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    socklen_t service_addr_len = sizeof(service_addr);
    if (recvfrom(sock_fd, buffer, 1024, 0, (struct sockaddr*)&service_addr, &service_addr_len) < 0) {
        perror("Recvfrom failed");
        exit(EXIT_FAILURE);
    }
}

void
f_bot_results_collect(int sock_fd, struct sockaddr_in service_addr, std::vector<uint32_t>& results)
{
    const char* message = "finish";
    char buffer[1024];
    socklen_t service_addr_len = sizeof(service_addr);
    if (sendto(sock_fd, message, strlen(message), 0, (struct sockaddr*)&service_addr, service_addr_len) < 0) {
        perror("Sendto failed");
        exit(EXIT_FAILURE);
    }
    while (true) {
        ssize_t n = recvfrom(sock_fd, buffer, 1024, 0, (struct sockaddr*)&service_addr, &service_addr_len);
        if (n < 0) {
            perror("Recvfrom failed");
            exit(EXIT_FAILURE);
        }
        buffer[n] = '\0';
        if (strcmp(buffer, "end") == 0) {
            break;
        }
        if (n == sizeof(uint32_t)) {
            uint32_t result;
            memcpy(&result, buffer, sizeof(uint32_t));
            results.push_back(result);
        }
    }
}

void
f_bot_beaver_test(float bot_ratio)
{
    int total_requests = 1000;
    std::vector<uint32_t> attack_vec;
    std::vector<uint32_t> estimate_attack_vec;
    srand(time(NULL));
    std::ofstream output_file("bot.txt");
    int sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd < 0) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in service_addr1, service_addr2;
    memset(&service_addr1, 0, sizeof(service_addr1));
    service_addr1.sin_family = AF_INET;
    service_addr1.sin_port = htons(10000);
    inet_pton(AF_INET, "192.168.100.1", &service_addr1.sin_addr);
    memset(&service_addr2, 0, sizeof(service_addr2));
    service_addr2.sin_family = AF_INET;
    service_addr2.sin_port = htons(10000);
    inet_pton(AF_INET, "192.168.100.2", &service_addr2.sin_addr);

    int count = 0;
    for (int i = 0; i < total_requests; ++i) {
        if ((double)rand() / RAND_MAX < bot_ratio) {
            f_bot_udp_req(sock_fd, service_addr2, "Request");
            count++;
            attack_vec.push_back(1);
        } else {
            f_bot_udp_req(sock_fd, service_addr1, "Request");
            f_bot_udp_req(sock_fd, service_addr2, "Request");
            attack_vec.push_back(0);
        }
        usleep(1000);
    }
    std::vector<uint32_t> results_vec1;
    std::vector<uint32_t> results_vec2;
    f_bot_results_collect(sock_fd, service_addr1, results_vec1);
    f_bot_results_collect(sock_fd, service_addr2, results_vec2);
    uint32_t bot_count = 0;
    uint32_t vec_index = 0;
    while (vec_index < results_vec1.size()) {
        if (results_vec1[vec_index] <= results_vec2[vec_index + bot_count]) {
            estimate_attack_vec.push_back(0);
            vec_index++;
        } else {
            estimate_attack_vec.push_back(1);
            bot_count++;
        }
    }
    output_file << results_vec1.size() << " " << results_vec2.size() << std::endl;
    output_file << "Bot Count: " << count << std::endl;

    uint32_t true_positive = 0;
    uint32_t false_positive = 0;
    uint32_t true_negative = 0;
    uint32_t false_negative = 0;
    for (uint32_t i = 0; i < attack_vec.size(); i++) {
        if (attack_vec[i] == 1 && estimate_attack_vec[i] == 1) {
            true_positive++;
        } else if (attack_vec[i] == 0 && estimate_attack_vec[i] == 1) {
            false_positive++;
        } else if (attack_vec[i] == 0 && estimate_attack_vec[i] == 0) {
            true_negative++;
        } else {
            false_negative++;
        }
    }
    output_file << "True Positive: " << (float)true_positive / total_requests << std::endl;
    output_file << "False Positive: " << (float)false_positive / total_requests << std::endl;
    output_file << "True Negative: " << (float)true_negative / total_requests << std::endl;
    output_file << "False Negative: " << (float)false_negative / total_requests << std::endl;
    output_file.close();
}

void
f_bot_poll_test(float bot_ratio)
{
    int total_requests = 1000;
    std::vector<uint32_t> attack_vec;
    std::vector<uint32_t> estimate_attack_vec;
    srand(time(NULL));
    std::ofstream output_file("bot.txt");
    int sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd < 0) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in service_addr1, service_addr2;
    memset(&service_addr1, 0, sizeof(service_addr1));
    service_addr1.sin_family = AF_INET;
    service_addr1.sin_port = htons(10000);
    inet_pton(AF_INET, "192.168.100.1", &service_addr1.sin_addr);
    memset(&service_addr2, 0, sizeof(service_addr2));
    service_addr2.sin_family = AF_INET;
    service_addr2.sin_port = htons(10000);
    inet_pton(AF_INET, "192.168.100.2", &service_addr2.sin_addr);
    char message[] = "start";
    if (sendto(sock_fd, message, strlen(message), 0, (struct sockaddr*)&service_addr1, sizeof(service_addr1)) < 0) {
        perror("Sendto failed");
        exit(EXIT_FAILURE);
    }
    if (sendto(sock_fd, message, strlen(message), 0, (struct sockaddr*)&service_addr2, sizeof(service_addr2)) < 0) {
        perror("Sendto failed");
        exit(EXIT_FAILURE);
    }
    usleep(10000);
    int count = 0;
    for (int i = 0; i < total_requests; ++i) {
        if ((double)rand() / RAND_MAX < bot_ratio) {
            f_bot_udp_req(sock_fd, service_addr2, "RequestP");
            count++;
            attack_vec.push_back(1);
        } else {
            f_bot_udp_req(sock_fd, service_addr1, "RequestP");
            f_bot_udp_req(sock_fd, service_addr2, "RequestP");
            attack_vec.push_back(0);
        }
        usleep(1000);
    }
    std::vector<uint32_t> results_vec1;
    std::vector<uint32_t> results_vec2;
    f_bot_results_collect(sock_fd, service_addr1, results_vec1);
    f_bot_results_collect(sock_fd, service_addr2, results_vec2);
    uint32_t bot_count = 0;
    uint32_t vec_index = 0;
    std::cout << results_vec1.size() << " " << results_vec2.size() << std::endl;
    while (vec_index < results_vec1.size()) {
        if (results_vec1[vec_index] <= results_vec2[vec_index + bot_count]) {
            estimate_attack_vec.push_back(0);
            vec_index++;
        } else {
            estimate_attack_vec.push_back(1);
            bot_count++;
        }
    }
    output_file << results_vec1.size() << " " << results_vec2.size() << std::endl;
    output_file << "Bot Count: " << count << std::endl;
    uint32_t true_positive = 0;
    uint32_t false_positive = 0;
    uint32_t true_negative = 0;
    uint32_t false_negative = 0;
    for (uint32_t i = 0; i < attack_vec.size(); i++) {
        if (attack_vec[i] == 1 && estimate_attack_vec[i] == 1) {
            true_positive++;
        } else if (attack_vec[i] == 0 && estimate_attack_vec[i] == 1) {
            false_positive++;
        } else if (attack_vec[i] == 0 && estimate_attack_vec[i] == 0) {
            true_negative++;
        } else {
            false_negative++;
        }
    }
    output_file << "True Positive: " << (float)true_positive / total_requests << std::endl;
    output_file << "False Positive: " << (float)false_positive / total_requests << std::endl;
    output_file << "True Negative: " << (float)true_negative / total_requests << std::endl;
    output_file << "False Negative: " << (float)false_negative / total_requests << std::endl;
    output_file.close();
}

void
f_bot_laiyang_test(float bot_ratio)
{
    int total_requests = 1000;
    std::vector<uint32_t> attack_vec;
    std::vector<uint32_t> estimate_attack_vec;
    srand(time(NULL));
    std::ofstream output_file("bot.txt");
    int sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd < 0) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in service_addr1, service_addr2;
    memset(&service_addr1, 0, sizeof(service_addr1));
    service_addr1.sin_family = AF_INET;
    service_addr1.sin_port = htons(10000);
    inet_pton(AF_INET, "192.168.100.1", &service_addr1.sin_addr);
    memset(&service_addr2, 0, sizeof(service_addr2));
    service_addr2.sin_family = AF_INET;
    service_addr2.sin_port = htons(10000);
    inet_pton(AF_INET, "192.168.100.2", &service_addr2.sin_addr);
    usleep(10000);
    int count = 0;
    for (int i = 0; i < total_requests; ++i) {
        if ((double)rand() / RAND_MAX < bot_ratio) {
            f_bot_udp_req(sock_fd, service_addr2, "RequestP");
            count++;
            attack_vec.push_back(1);
        } else {
            f_bot_udp_req(sock_fd, service_addr1, "RequestP");
            f_bot_udp_req(sock_fd, service_addr2, "RequestP");
            attack_vec.push_back(0);
        }
        usleep(1000);
    }
    std::vector<uint32_t> results_vec1;
    std::vector<uint32_t> results_vec2;
    f_bot_results_collect(sock_fd, service_addr1, results_vec1);
    f_bot_results_collect(sock_fd, service_addr2, results_vec2);
    uint32_t bot_count = 0;
    uint32_t vec_index = 0;
    std::cout << results_vec1.size() << " " << results_vec2.size() << std::endl;
    while (vec_index < results_vec1.size()) {
        if (results_vec1[vec_index] <= results_vec2[vec_index + bot_count]) {
            estimate_attack_vec.push_back(0);
            vec_index++;
        } else {
            estimate_attack_vec.push_back(1);
            bot_count++;
        }
    }
    output_file << results_vec1.size() << " " << results_vec2.size() << std::endl;
    output_file << "Bot Count: " << count << std::endl;
    uint32_t true_positive = 0;
    uint32_t false_positive = 0;
    uint32_t true_negative = 0;
    uint32_t false_negative = 0;
    for (uint32_t i = 0; i < attack_vec.size(); i++) {
        if (attack_vec[i] == 1 && estimate_attack_vec[i] == 1) {
            true_positive++;
        } else if (attack_vec[i] == 0 && estimate_attack_vec[i] == 1) {
            false_positive++;
        } else if (attack_vec[i] == 0 && estimate_attack_vec[i] == 0) {
            true_negative++;
        } else {
            false_negative++;
        }
    }
    output_file << "True Positive: " << (float)true_positive / total_requests << std::endl;
    output_file << "False Positive: " << (float)false_positive / total_requests << std::endl;
    output_file << "True Negative: " << (float)true_negative / total_requests << std::endl;
    output_file << "False Negative: " << (float)false_negative / total_requests << std::endl;
    output_file.close();
}