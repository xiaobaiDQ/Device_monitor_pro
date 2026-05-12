# 嵌入式 Linux 设备状态监控与远程控制系统

## 1. 项目简介

本项目是一个基于 Linux C 开发的嵌入式设备状态监控与远程控制系统，采用多进程架构实现设备状态采集、进程间通信、日志记录、网络上报和远程命令控制。

系统运行后，collector 进程负责采集 CPU、内存、温度、磁盘和网络速率等设备状态；network 进程负责从共享内存读取状态数据，并通过 TCP 上报至服务端；logger 进程负责统一记录系统日志；manager 进程负责创建、监控和重启子进程。

本项目适用于嵌入式 Linux 设备、边缘计算设备、机器人控制主机、树莓派和小型 Linux 工控机等场景。

---

device_monitor_pro/
├── app/
│   ├── manager.c          # 主控进程
│   ├── collector.c        # 状态采集进程
│   ├── network.c          # TCP 通信进程
│   └── logger.c           # 日志进程
│
├── include/
│   ├── common.h           # 公共数据结构
│   ├── shm.h              # 共享内存接口
│   ├── sem.h              # 信号量接口
│   ├── msgq.h             # 消息队列接口
│   ├── log.h              # 日志接口
│   ├── config.h           # 配置文件接口
│   ├── alarm.h            # 告警模块接口
│   ├── protocol.h         # 通信协议接口
│   ├── tcp_client.h       # TCP 客户端接口
│   ├── cpu.h              # CPU 采集接口
│   ├── memory.h           # 内存采集接口
│   ├── temp.h             # 温度采集接口
│   ├── disk.h             # 磁盘采集接口
│   └── network_stat.h     # 网络速率采集接口
│
├── src/
│   ├── shm.c              # 共享内存实现
│   ├── sem.c              # 信号量实现
│   ├── msgq.c             # 消息队列实现
│   ├── log.c              # 日志发送实现
│   ├── config.c           # 配置文件解析
│   ├── alarm.c            # 告警检测实现
│   ├── protocol.c         # 协议封装实现
│   └── tcp_client.c       # TCP 客户端实现
│
├── monitor/
│   ├── cpu.c              # CPU 使用率采集
│   ├── memory.c           # 内存使用率采集
│   ├── temp.c             # 温度采集
│   ├── disk.c             # 磁盘使用率采集
│   └── network_stat.c     # 网络收发速率采集
│
├── test/
│   ├── test_cpu.c
│   ├── test_cpu_fast.c
│   ├── test_memory.c
│   ├── test_temp.c
│   ├── test_disk.c
│   ├── test_network_stat.c
│   ├── test_network_stat_fast.c
│   ├── test_msgq.c
│   ├── test_config.c
│   ├── test_protocol.c
│   ├── test_tcp_client.c
│   └── test_tcp_server.c
│
├── config/
│   └── monitor.conf       # 配置文件
│
├── logs/
│   └── monitor.log        # 日志文件
│
├── Makefile
└── README.md


## 2. 项目功能

### 2.1 系统状态采集

当前支持采集以下设备状态：

- CPU 使用率
- 内存使用率
- 系统温度
- 磁盘使用率
- 网络接收速率 RX
- 网络发送速率 TX

### 2.2 多进程架构

系统包含以下核心进程：

- manager：主控进程，负责创建和管理子进程
- collector：采集进程，负责读取系统状态数据
- network：通信进程，负责 TCP 数据上报和命令接收
- logger：日志进程，负责统一写入日志文件

### 2.3 IPC 进程间通信

项目使用多种 Linux IPC 机制：

- 共享内存：collector 与 network 之间共享系统状态数据
- 信号量：保护共享内存，避免并发读写冲突
- 消息队列：业务进程向 logger 异步发送日志消息

### 2.4 TCP 远程通信

network 进程作为 TCP 客户端，支持：

- 周期性上传设备状态
- 接收服务端远程命令
- 返回 ACK / ERR 执行结果
- 网络断开后自动重连

### 2.5 远程命令控制

服务端当前支持以下命令：

```text
GET_STATUS
SET_INTERVAL 2
STOP


                         +----------------+
                         |    manager     |
                         |  主控进程       |
                         +--------+-------+
                                  |
          +-----------------------+-----------------------+
          |                       |                       |
          v                       v                       v
+----------------+       +----------------+       +----------------+
|   collector    |       |    network     |       |     logger     |
|   采集进程      |       |   通信进程      |       |   日志进程      |
+-------+--------+       +--------+-------+       +--------+-------+
        |                         |                        ^
        |                         |                        |
        v                         v                        |
+----------------+       +----------------+                |
|  共享内存 shm   |<----->| TCP client     |                |
+----------------+       +----------------+                |
        ^                         |                        |
        |                         v                        |
+----------------+       +----------------+                |
|  信号量 sem     |       | TCP server     |                |
+----------------+       +----------------+                |
                                                          |
+---------------------------------------------------------+
|                  消息队列 msgq                           |
+---------------------------------------------------------+

collector
    ↓
读取 /proc /sys 系统信息
    ↓
写入共享内存
    ↓
network 读取共享内存
    ↓
构造 JSON 数据
    ↓
通过 TCP 上传至服务端

manager / collector / network
    ↓
log_info / log_warn / log_error
    ↓
System V 消息队列
    ↓
logger 进程
    ↓
logs/monitor.log

TCP server 输入命令
    ↓
network 接收命令
    ↓
protocol 解析命令
    ↓
执行对应操作
    ↓
返回 ACK / ERR

## 2.配置文件示例

run_time=300
sample_interval=5

cpu_threshold=80
mem_threshold=85
temp_threshold=70

server_ip=127.0.0.1
server_port=8888

network_iface=enp3s0

log_file=logs/monitor.log