#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <math.h>
#include <numeric>
#include <ostream>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#define MAXSIZE 512

int64_t get_network_timestamp() {

    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

int64_t std_dev(const std::vector<int64_t>& vec) {

    int64_t mean = std::accumulate(vec.begin(), vec.end(), 0.0);

    int64_t var;
    for (int64_t x : vec) {

        var += (x - mean) * (x - mean);
    }

    var /= vec.size();

    // could microptimize into heron's method
    return std::sqrt(var);
}

int main() {

    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    struct sockaddr_in addr = {};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(fd, (const struct sockaddr*)&addr, sizeof(addr)) < 0) {

        perror("Bind failed");
        close(fd);
        return -1;
    }

    std::cout << "im doing smth" << std::endl;

    char buf[MAXSIZE] = {'\0'};
    int64_t cur_packet = -1;
    int64_t dropped_packets = 0;
    // let the protocol be (NEWC (4 bytes), num packet (size_t bytes))
    // change this to be message oriented, since udp uses the message system
    //
    struct pollfd pollfds[1];
    pollfds[0].fd = fd;
    pollfds[0].events = POLLIN;

    int64_t min_latency = std::numeric_limits<int64_t>::max();
    int64_t max_latency = -1;
    int64_t total_latency = 0;
    int64_t latency;

    std::vector<int64_t> arrivals;
    arrivals.reserve(1000);
    arrivals.push_back(get_network_timestamp());
    std::vector<int64_t> deltas;
    deltas.reserve(1000);

    while (true) {

        struct sockaddr_in client_addr;
        socklen_t a = sizeof(client_addr);

        int64_t d[2];
        int r = poll(pollfds, 1, 5000);

        if (r == 0) {

            break;
        }

        int n = recvfrom(fd, (char*)d, sizeof(int64_t) * 2, 0, (struct sockaddr*)&client_addr, &a);
        latency = get_network_timestamp() - d[1];

        arrivals.push_back(d[1]);

        deltas.push_back(arrivals.back() - arrivals[arrivals.size() - 2]);

        if (d[0] == cur_packet + 1) {
            cur_packet = d[0];

            total_latency += latency;
            min_latency = std::min(latency, min_latency);
            max_latency = std::max(latency, max_latency);
        }

        else if (d[1] > cur_packet + 1) {
            std::cout << std::dec;
            std::cout << "I just dropped packets from " << cur_packet << " to " << d[0]
                      << std::endl;
            dropped_packets += d[0] - cur_packet - 1;
            cur_packet = d[0];

            total_latency += latency;
            min_latency = std::min(latency, min_latency);
            max_latency = std::max(max_latency, latency);
        }

        else {
            std::cout << std::dec;
            std::cout << "Recieved packet " << d << "Out of order" << std::endl;
        }
        std::cout << std::dec;
        std::cout << "On packet " << cur_packet << std::endl;
    }

    dropped_packets += 1000 - cur_packet;
    std::cout << "dropped packets " << dropped_packets << std::endl;
    std::cout << "avg latency " << total_latency / 1000 << std::endl;
    std::cout << "min latency " << min_latency << std::endl;
    std::cout << "max latency " << max_latency << std::endl;
    std::cout << "jitter (std deviation of arrival deltas) " << std_dev(deltas) << std::endl;

    return 0;
}
