#include "test.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>


void receive_monitoring_data(int listen_sock, char* target_ip) {

    char dir_path[256];
    snprintf(dir_path, sizeof(dir_path), "../monitor_data/%s", target_ip);

    // 创建子文件夹
    if (mkdir(dir_path, 0777) && errno != EEXIST) {
        perror("Failed to create directory");
        return;
    }
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y%m%d%H%M%S", t);

    char buffer[256];
    int bytes_received = sgx_read_sock(listen_sock, buffer, sizeof(buffer) - 1);
    if (bytes_received < 0) {
        perror("Receive failed");
    } else {
        buffer[bytes_received] = '\0';  
        printf("Received message from Target: %s\n", buffer);

        float cpu_usage, memory_usage;
        if (sscanf(buffer, "{\"cpu_usage\": %f, \"memory_usage\": %f}",
                   &cpu_usage, &memory_usage) == 2) {
            printf("Parsed CPU Usage: %.2f%%\n", cpu_usage);
            printf("Parsed Memory Usage: %.2f%%\n", memory_usage);

            // 可以将数据保存到文件或数据库中
            char filename[256];
            snprintf(filename, sizeof(filename), "%s/monitor_result_%s.json", dir_path, time_str);

            // 打开文件
            FILE *file = fopen(filename, "w");
            if (file != NULL) {
                fprintf(file, "{\"time\": %s, \"cpu_usage\": %.2f, \"memory_usage\": %.2f}\n", time_str, cpu_usage, memory_usage);
                fclose(file);
            }
        } else {
            fprintf(stderr, "Failed to parse the received JSON message.\n");
        }
    }
}

void enclave_main(int argc, char **argv)
{
    const char *target_ip = argv[2];
    int handshake_port = 8024;  // 定义一个用于握手的端口
    int target_port = 8025, target_port2 = 8028, ret;
    const char *challenge = "Are you SGX enclave?";

    // 发送握手信息
    int handshake_sock = sgx_connect_server(target_ip, handshake_port);
    if (handshake_sock < 0) {
        perror("Handshake socket connection failed");
        sgx_exit(NULL);
    }

    const char *handshake_message = "Handshake Initiated";
    sgx_write_sock(handshake_sock, (void *)handshake_message, strlen(handshake_message));
    close(handshake_sock);

    sleep(1);

    // 进行远程认证
    ret = sgx_remote_attest_challenger(target_ip, target_port, challenge);
    if(ret == 1) {
        puts("Remote Attestation Success!");

        int listen_sock = sgx_make_server(target_port2);
        if (listen_sock < 0) {
            perror("Socket creation failed");
            sgx_exit(NULL);
        }

        puts("Waiting for a message from Target...");
        receive_monitoring_data(listen_sock, target_ip);

        // char buffer[256];
        // int bytes_received = sgx_read_sock(listen_sock, buffer, sizeof(buffer) - 1);
        // if (bytes_received < 0) {
        //     perror("Receive failed");
        // } else {
        //     buffer[bytes_received] = '\0';  
        //     printf("Received message from Target: %s\n", buffer);
        // }

        close(listen_sock);
    } else {
        puts("Remote Attestation Fail!");
    }

    sgx_exit(NULL);
}
