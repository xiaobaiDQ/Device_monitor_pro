# Device Monitor Pro

`Device Monitor Pro` 是一个基于 Linux C 开发的设备状态监控与远程控制系统，适用于嵌入式 Linux 设备、边缘计算设备、树莓派、小型工控机等场景。

项目通过多进程架构实现系统状态采集、进程间通信、日志记录、TCP 上报以及远程命令控制，整体设计偏向工程化和可扩展。

## 项目目标

本项目希望解决以下问题：

- 周期性采集设备运行状态
- 将状态数据实时上报到远程服务端
- 支持服务端下发控制命令
- 使用统一日志系统记录运行信息
- 在子进程异常退出时自动恢复服务

## 核心功能

- 采集 CPU 使用率
- 采集内存使用率
- 采集系统温度
- 采集磁盘使用率
- 采集网络接收 / 发送速率
- 通过 TCP 周期性上传设备状态
- 接收并处理服务端命令
- 统一写入日志文件
- 使用进程守护机制提升稳定性

## 项目架构

项目采用 4 个核心进程协作：

- `manager`：主控进程，负责创建共享资源、启动子进程、监控子进程状态、异常重启与退出清理
- `collector`：采集进程，负责读取系统指标并写入共享内存
- `network`：通信进程，负责从共享内存读取状态并通过 TCP 上报，同时接收服务端命令
- `logger`：日志进程，负责消费消息队列并统一写入日志文件
- `web`：网页监控进程，负责读取共享内存并提供一个最小可用的浏览器监控页面

### 数据流

```text
collector
  ↓
采集系统状态
  ↓
写入共享内存
  ↓
network 读取状态
  ↓
封装 JSON
  ↓
TCP 上报到服务端
  ↓
接收服务端命令
  ↓
解析并执行命令
  ↓
返回 ACK / ERR
```

### IPC 设计

项目使用了三种 Linux IPC 机制：

- 共享内存：用于 `collector` 和 `network` 共享最新状态
- 信号量：用于保护共享内存，避免并发冲突
- 消息队列：用于业务进程异步发送日志到 `logger`

## 支持的远程命令

当前服务端支持以下命令：

- `GET_STATUS`：请求当前设备状态
- `SET_INTERVAL <n>`：设置上报周期，单位秒，当前实现支持 `1 ~ 60`
- `STOP`：通知系统停止运行

## 目录结构

```text
device_monitor_pro/
├── app/            # 主程序入口：manager、collector、network、logger、web
├── include/        # 头文件：公共结构、IPC、协议、配置、采集接口
├── src/            # 基础模块实现：配置、日志、协议、TCP、IPC
├── monitor/        # 各类系统指标采集实现
├── test/           # 单元测试与功能测试
├── config/         # 配置文件
├── logs/           # 日志输出目录
├── Makefile        # 构建脚本
└── README.md       # 项目说明
```

## 配置说明

默认配置文件路径为：`config/monitor.conf`

示例配置：

```ini
run_time=300
sample_interval=5

cpu_threshold=80
mem_threshold=85
temp_threshold=70

server_ip=127.0.0.1
server_port=8888

network_iface=enp3s0

log_file=logs/monitor.log
```

### 配置项说明

- `run_time`：manager 运行时间，单位秒
- `sample_interval`：collector 采集周期，单位秒
- `cpu_threshold`：CPU 告警阈值
- `mem_threshold`：内存告警阈值
- `temp_acpitz_1_threshold`：温度告警阈值
- `temp_acpitz_2_threshold`：温度告警阈值
- `temp_cpu_threshold`：温度告警阈值
- `server_ip`：TCP 服务端地址
- `server_port`：TCP 服务端端口
- `network_iface`：网络接口名
- `log_file`：日志文件路径

## 运行前准备

### 依赖环境

- Linux 系统
- GCC 或 Clang
- Make
- 标准 C 库
- 支持 `/proc`、`/sys` 的 Linux 环境

### 编译

通常可直接使用 Makefile 构建：

```bash
make
```

如果你的 Makefile 提供了清理目标，也可以使用：

```bash
make clean
```

## 运行方式

在完成编译后，通常需要按以下顺序启动：

1. 启动服务端（用于接收状态和下发命令）
2. 启动本项目主程序 `manager`
3. 由 `manager` 自动拉起 `collector`、`network`、`logger`

如果你的构建产物就在项目根目录，并且可执行文件名与代码中一致，那么 `manager` 会通过 `execl()` 拉起：

- `./collector`
- `./network`
- `./logger`

网页监控界面需要单独启动：

```bash
./web
```

默认地址是 `http://127.0.0.1:8080`。

## 测试说明

`test/` 目录下提供了多个测试程序，用于验证各个模块：

- CPU 采集
- 内存采集
- 温度采集
- 磁盘采集
- 网络速率采集
- 配置解析
- 协议解析与封装
- TCP 客户端 / 服务端
- 消息队列

你可以按 Makefile 中的定义单独编译或运行这些测试程序。

## 日志说明

日志由 `logger` 进程统一写入：

- 默认输出到 `logs/monitor.log`
- 业务进程不直接写文件
- 这种方式可以降低进程间耦合，并避免频繁文件 I/O 影响采集和通信性能

## 目前实现特点

- 多进程架构清晰
- 共享内存读写配合信号量保护
- TCP 通信支持重连
- 支持简单的远程控制命令
- 日志集中管理
- 已提供最小可用网页监控界面
- 适合嵌入式 Linux 监控场景

## 后续可扩展方向

如果你后续继续完善，比较适合加入以下能力：

- 告警阈值触发与上报
- 更完善的服务端协议
- 更健壮的重连与心跳机制
- 配置热加载
- 进程 watchdog 机制
- 更完整的安装与部署脚本

## 许可证

当前项目未单独声明许可证。如需开源发布，建议补充 LICENSE 文件并明确授权方式。
