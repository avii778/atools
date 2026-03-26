#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <chrono>
#include <cstdint>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// idek man!
int64_t get_network_timestamp() {

    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}
int main() {

    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));

    struct sockaddr_in addr = {};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (int n = connect(fd, (const struct sockaddr*)&addr, sizeof(addr)) < 0) {

        perror("lol");
        return -1;
    }

    for (int i = 0; i < 1000; i++) {
        int64_t buf[2];
        buf[0] = (int64_t)i;
        buf[1] = get_network_timestamp();
        int g = send(fd, (const char*)buf, sizeof(int64_t) * 2, 0);
    }

    return 0;
}
