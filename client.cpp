#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <chrono>
#include <cstdint>
#include <cstring>
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

    int n = connect(fd, (const struct sockaddr*)&addr, sizeof(addr));
    if (n < 0) {
        perror("lol");
        return -1;
    }

    // hmm maybe make it (size)(num_packet)(latency)(packet)
    int i = 0;
    int b;
    while (true) {
        b = 0;

        // 256 packets should be fine (256 * 16)
        uint8_t buf[2 * sizeof(int) + sizeof(int64_t) + 512];
        memcpy(buf + sizeof(int), &i, sizeof(int));
        int64_t time = get_network_timestamp();
        memcpy(buf + 2 * sizeof(int), &time, sizeof(int64_t));

        // try to read the 256 audio packets
        int num_read = 0;
        uint16_t packet[256] = {0};

        while (num_read < (int)sizeof(packet)) {
            int n = read(STDIN_FILENO, (uint8_t*)packet + num_read, sizeof(packet) - num_read);

            if (n < 0) {
                perror("read");
                return 1;
            }

            if (n == 0) {
                break;
            }

            num_read = n;
        }

        if (num_read == 0) {
            b = 1;
        }

        memcpy(buf, &num_read, sizeof(int));
        memcpy(buf + 2 * sizeof(int) + sizeof(int64_t), packet, sizeof(packet));
        int total = 2 * sizeof(int) + sizeof(int64_t) + num_read;
        // may god help us all
        int g = send(fd, (const char*)buf, total, 0);

        if (g < 0) {
            printf("IT WAS ME!");
        }
        i++;
        if (b) {
            break;
        }
    }

    return 0;
}
