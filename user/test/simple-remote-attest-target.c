// target
#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define CPUINFO_PATH "/proc/cpuinfo"
#define STAT_PATH "/proc/stat"
#define MEMINFO_PATH "/proc/meminfo"
#define MAX_CHARS 512 

void send_monitoring_data(int sock, float cpu_usage, float memory_usage) {
    char usage_msg[256];  // 增加缓冲区大小以适应 JSON 数据
    snprintf(usage_msg, sizeof(usage_msg),
             "{\"cpu_usage\": %.2f, \"memory_usage\": %.2f}",
             cpu_usage, memory_usage);
    
    sgx_write_sock(sock, (void *)usage_msg, strlen(usage_msg));
}

char* get_cpuinfo_string() {
    FILE *cpuinfo_file = fopen(CPUINFO_PATH, "r");
    if (cpuinfo_file == NULL) {
        perror("Failed to open /proc/cpuinfo");
        return NULL;
    }

    fseek(cpuinfo_file, 0, SEEK_END);
    long file_size = ftell(cpuinfo_file);
    rewind(cpuinfo_file);

    char *cpuinfo_data = (char *)malloc(MAX_CHARS + 1);
    if (cpuinfo_data == NULL) {
        perror("Failed to allocate memory for CPU info");
        fclose(cpuinfo_file);
        return NULL;
    }

    size_t read_size = fread(cpuinfo_data, 1, MAX_CHARS, cpuinfo_file);
    if (read_size == 0) {
        perror("Failed to read CPU info");
        free(cpuinfo_data);
        fclose(cpuinfo_file);
        return NULL;
    }

    cpuinfo_data[read_size] = '\0';

    fclose(cpuinfo_file);

    return cpuinfo_data;
}

float get_cpu_usage() {
    FILE *stat_file;
    unsigned long long int user, nice, system, idle, iowait, irq, softirq;
    unsigned long long int total_time1, total_time2, idle_time1, idle_time2;

    stat_file = fopen(STAT_PATH, "r");
    if (stat_file == NULL) {
        perror("Failed to open /proc/stat");
        return -1;
    }

    fscanf(stat_file, "cpu %llu %llu %llu %llu %llu %llu %llu", 
           &user, &nice, &system, &idle, &iowait, &irq, &softirq);
    fclose(stat_file);

    total_time1 = user + nice + system + idle + iowait + irq + softirq;
    idle_time1 = idle + iowait;

    sleep(1); // Wait for a second to get a difference

    stat_file = fopen(STAT_PATH, "r");
    if (stat_file == NULL) {
        perror("Failed to open /proc/stat");
        return -1;
    }

    fscanf(stat_file, "cpu %llu %llu %llu %llu %llu %llu %llu", 
           &user, &nice, &system, &idle, &iowait, &irq, &softirq);
    fclose(stat_file);

    total_time2 = user + nice + system + idle + iowait + irq + softirq;
    idle_time2 = idle + iowait;

    float total_diff = total_time2 - total_time1;
    float idle_diff = idle_time2 - idle_time1;

    return (100.0 * (total_diff - idle_diff)) / total_diff;
}

float get_memory_usage() {
    FILE *meminfo_file = fopen(MEMINFO_PATH, "r");
    if (meminfo_file == NULL) {
        perror("Failed to open /proc/meminfo");
        return -1;
    }

    unsigned long long int total_mem = 0, free_mem = 0, available_mem = 0;
    char buffer[MAX_CHARS];

    while (fgets(buffer, sizeof(buffer), meminfo_file)) {
        if (sscanf(buffer, "MemTotal: %llu kB", &total_mem) == 1 ||
            sscanf(buffer, "MemFree: %llu kB", &free_mem) == 1 ||
            sscanf(buffer, "MemAvailable: %llu kB", &available_mem) == 1) {
            continue;
        }
    }

    fclose(meminfo_file);

    if (total_mem == 0) {
        perror("Failed to read total memory");
        return -1;
    }

    // Calculate memory usage percentage
    unsigned long long int used_mem = total_mem - available_mem;
    return (100.0 * used_mem) / total_mem;
}

void enclave_main(int argc, char **argv)
{
    const char *challenger_ip = argv[2];
    char *message = get_cpuinfo_string();
    int challenger_port = 8025, quote_port = 8026, ret;
    char *conf = "../user/demo/simple-remote-attest-quote.conf";
    
    // Get CPU usage
    float cpu_usage = get_cpu_usage();
    if(cpu_usage < 0) {
        perror("Failed to get CPU usage");
        sgx_exit(NULL);
    }
    printf("CPU Usage: %.2f%%\n", cpu_usage);

    // Get memory usage
    float memory_usage = get_memory_usage();
    if(memory_usage < 0) {
        perror("Failed to get memory usage");
        sgx_exit(NULL);
    }
    printf("Memory Usage: %.2f%%\n", memory_usage);
    ret = sgx_remote_attest_target(challenger_port, quote_port, conf, "127.0.0.1");
    if(ret == 1) {
        puts("Remote Attestation Success!");

        sleep(1); 
        // puts(challenger_ip);
        int sock = sgx_connect_server(challenger_ip, 8028); // 挑战者的IP
        if (sock < 0) {
            perror("Connection to challenger failed");
            sgx_exit(NULL);
        }

        // sgx_write_sock(sock, (void *)message, strlen(message));
        
        // Send CPU and memory usage to the server

        // char usage_msg[100];
        // snprintf(usage_msg, sizeof(usage_msg), "CPU Usage: %.2f%%, Memory Usage: %.2f%%", cpu_usage, memory_usage);
        // sgx_write_sock(sock, (void *)usage_msg, strlen(usage_msg));
        send_monitoring_data(sock, cpu_usage, memory_usage);
        close(sock);
    } else {
        puts("Remote Attestation Fail!");
    }

    sgx_exit(NULL);
}
