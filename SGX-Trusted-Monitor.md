# SGX-Trusted-Monitor 使用说明

## 作为服务端

1. 进入 `monitor_target` 目录：
   ```bash
   cd monitor_target
   ```
2. 启动监听服务，替换 `[监控端的IP]` 为实际监控端的 IP 地址：
   ```bash
   ./hand_listener [监控端的IP]
   ```

## 作为监控端

1. 进入 `monitor_server` 目录：
   ```bash
   cd monitor_server
   ```
2. 启动 Flask 应用：
   ```bash
   python3 app.py
   ```

## monitor_server 文件夹内容说明

`monitor_server` 文件夹包含了实现WEB界面的前后端代码。

### 文件说明

1. **app.py**
   - 后端应用，使用 Python 3.5 和 Flask 1.1.4 构建。
   - 提供以下 API 路由：
     - `/`：返回前端页面 `index.html`。
     - `/api/resource-usage/current?ip=<ip>`：监控指定 IP 的当前资源使用情况。
     - `/api/resource-usage/history?ip=<ip>`：获取指定 IP 的历史资源使用数据。
     - `/api/resource-usage/clear?ip=<ip>`：清空指定 IP 的历史监控数据。
     - `/api/resource-usage/logs`：提供监控任务的执行日志。

2. **index.html**
   - 前端界面，提供用户交互功能：
     - 输入 IP 地址进行资源监控。
     - 查看当前和历史资源使用情况。
     - 清空历史记录。
     - 实时日志显示监控任务执行过程。

### 环境配置

1. **安装 Python 依赖**：
   - 使用 `requirements.txt` 安装所需依赖：
     ```bash
     pip install -r requirements.txt
     ```

2. **启动 Flask 应用**：
   - 启动应用，服务器将运行在 `http://0.0.0.0:5000`：
     ```bash
     python3 app.py
     ```

3. **访问前端页面**：
   - 在浏览器中打开 `http://<your-server-ip>:5000`，其中 `<your-server-ip>` 替换为你的服务器 IP 地址。

### 依赖项

- Python 3.5.2
- Flask 1.1.4

## monitor_target 文件夹内容说明

`monitor_target` 文件夹包含被监控端的代码和脚本。

### 文件说明

1. **hand_listener.c**
   - 用于监听特定端口的 C 程序。
   - 当接收到握手消息时，会触发 `monitor_targets.sh` 脚本，执行远程认证和资源监控操作，并传递目标 IP 地址。

2. **monitor_targets.sh**
   - 该脚本启动 `user/demo/simple-remote-attest-quote.sgx` 和 `user/test/simple-remote-attest-target.c`。
   - 这两个文件分别作为 quoting enclave 和 target enclave，用于进行远程认证和内部资源监控。

## monitor_data 文件夹说明

- **monitor_data**：用于存储各个 IP 的历史计算资源数据。

## 重新编译 `user` 目录下的文件

若需重新编译 `user` 目录下的文件，执行以下步骤：

1. 进入 `user` 目录并清理编译文件：
   ```bash
   cd user
   make clean
   cd ..
   ```
2. 重新编译 `user` 目录下的代码：
   ```bash
   make -C user
   ```
3. 使用 `opensgx` 工具编译和签署 SGX 文件：
   ```bash
   ./opensgx -k
   ./opensgx -c user/demo/simple-remote-attest-quote.c
   ./opensgx -s user/demo/simple-remote-attest-quote.sgx --key sign.key
   ```
4. **sign.key** 文件为密钥，用于签署 SGX 文件。

## 其他内容

- 有关 OpenSGX 的详细使用说明和教程，请参考主文件夹下的 `Opensgx_tutorial`。
