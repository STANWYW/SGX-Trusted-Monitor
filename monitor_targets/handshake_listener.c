#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define HANDSHAKE_PORT 8024
#define BUFFER_SIZE 256

int start_listening(int port) {
    int sockfd;
    struct sockaddr_in servaddr;

    // 创建套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // 绑定 IP 和端口
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 5) < 0) {
        perror("Listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

void handle_handshake(int client_sock, const char *ip_address) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("Received handshake message: %s\n", buffer);

        // 构建执行 monitor.sh 脚本的命令，并传递 IP 地址
        char command[BUFFER_SIZE + 64];
        snprintf(command, sizeof(command), "./monitor_targets.sh %s", ip_address);

        int ret = system(command);
        if (ret == -1) {
            perror("Failed to execute monitor.sh");
        } else {
            printf("Executed monitor.sh with IP address %s successfully\n", ip_address);
        }
    } else {
        perror("Receive failed");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <IP_ADDRESS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip_address = argv[1];

    int listen_sock = start_listening(HANDSHAKE_PORT);
    printf("Listening on port %d for handshake messages...\n", HANDSHAKE_PORT);

    while (1) {
        struct sockaddr_in clientaddr;
        socklen_t len = sizeof(clientaddr);

        int client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        handle_handshake(client_sock, ip_address);
        close(client_sock);
    }

    close(listen_sock);
    return 0;
}

