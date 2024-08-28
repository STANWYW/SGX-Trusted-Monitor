#!/bin/bash

# 从命令行获取第一个参数，即 IP 地址
IP_ADDRESS=$1
echo "The IP address provided is: $IP_ADDRESS"
# 执行 SGX 远程认证
(
  cd .. 
  ./opensgx user/demo/simple-remote-attest-quote.sgx user/demo/simple-remote-attest-quote.conf
) &  # 后台运行

# 执行测试脚本，并将 IP 地址传递给脚本的参数位置
(
  cd ../user
  ./test.sh test/simple-remote-attest-target $IP_ADDRESS
) &  # 后台运行

# 等待所有后台进程完成
wait

