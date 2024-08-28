import re
import subprocess
from flask import Flask, jsonify, request, send_from_directory, Response
import os
import json

app = Flask(__name__, static_folder='.')

# 配置监控数据的基本路径
MONITOR_DATA_PATH = "../monitor_data"
LOG_FILE_PATH = "monitor.log"  # 定义日志文件路径

# 返回主页（index.html）
@app.route('/')
def index():
    return send_from_directory(app.static_folder, 'index.html')

# 获取指定 IP 的最新资源使用情况
@app.route('/api/resource-usage/current', methods=['GET'])
def get_current_usage():
    ip = request.args.get('ip')
    if not ip:
        return jsonify({"error": "No IP provided"}), 400

    # 执行 Bash 脚本，并将输出写入日志文件
    with open(LOG_FILE_PATH, 'w') as log_file:
        try:
            command = 'cd ../user && ./test.sh test/simple-remote-attest-challenger {}'.format(ip)
            process = subprocess.Popen(
                ['bash', '-c', command],
                stdout=log_file,
                stderr=subprocess.STDOUT,
                universal_newlines=True
            )
            process.wait()
        except subprocess.CalledProcessError as e:
            return jsonify({"error": "Failed to execute monitoring script"}), 500

    dir_path = os.path.join(MONITOR_DATA_PATH, ip)
    if not os.path.exists(dir_path):
        return jsonify({"error": "No data for this IP"}), 404

    # 找到所有符合 monitor_result_YYYYMMDD_HHMMSS.json 格式的 JSON 文件
    json_files = [f for f in os.listdir(dir_path) if re.match(r'monitor_result_\d{14}\.json', f)]
    if not json_files:
        return jsonify({"error": "No data files found"}), 404

    # 按照时间戳部分对文件名进行排序，选择最新的文件
    latest_file = max(json_files, key=lambda f: re.search(r'\d{14}', f).group())

    latest_file_path = os.path.join(dir_path, latest_file)

    with open(latest_file_path, 'r') as f:
        data = json.load(f)

    return jsonify(data)

# 获取指定 IP 的历史资源使用情况
@app.route('/api/resource-usage/history', methods=['GET'])
def get_historical_usage():
    ip = request.args.get('ip')
    if not ip:
        return jsonify({"error": "No IP provided"}), 400

    dir_path = os.path.join(MONITOR_DATA_PATH, ip)
    if not os.path.exists(dir_path):
        return jsonify({"error": "No data for this IP"}), 404

    # 读取目录下的所有 JSON 文件
    json_files = sorted([f for f in os.listdir(dir_path) if f.endswith('.json')])
    if not json_files:
        return jsonify({"error": "No data files found"}), 404

    history_data = []
    for json_file in json_files:
        file_path = os.path.join(dir_path, json_file)
        with open(file_path, 'r') as f:
            data = json.load(f)
            history_data.append(data)

    return jsonify(history_data)

# 清空指定 IP 的历史资源记录
@app.route('/api/resource-usage/clear', methods=['DELETE'])
def clear_history():
    ip = request.args.get('ip')
    if not ip:
        return jsonify({"error": "No IP provided"}), 400

    dir_path = os.path.join(MONITOR_DATA_PATH, ip)
    if not os.path.exists(dir_path):
        return jsonify({"error": "No data for this IP"}), 404

    # 删除该 IP 目录下的所有 JSON 文件
    try:
        for file_name in os.listdir(dir_path):
            file_path = os.path.join(dir_path, file_name)
            if file_name.endswith('.json'):
                os.remove(file_path)
        return '', 204  # 返回 204 No Content 表示删除成功
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# 提供日志文件内容
@app.route('/api/resource-usage/logs', methods=['GET'])
def get_logs():
    def generate():
        with open(LOG_FILE_PATH, 'r') as log_file:
            for line in log_file:
                yield line

    return Response(generate(), mimetype='text/plain')

# 启动 Flask 应用
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
