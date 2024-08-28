#include <sgx.h>
#include <sgx-user.h>
#include <sgx-kern.h>
#include <sgx-lib.h>
#include <sgx-utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define EXPONENT 3
#define KEY_SIZE 128

void enclave_main()
{
    int target_port = 8026, ret;

    ret = sgx_remote_attest_quote(target_port);
    if(ret == 1) {
        puts("Remote Attestation Success!");

        FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
        if (cpuinfo == NULL) {
            perror("Unable to open /proc/cpuinfo");
            sgx_exit(NULL);
        }

        char buffer[256];
        while (fgets(buffer, sizeof(buffer), cpuinfo)) {
            puts(buffer);
        }

        fclose(cpuinfo);
    } else {
        puts("Remote Attestation Fail!");
    }
    
    sgx_exit(NULL);
}

