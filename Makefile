# 编译器
CC = gcc

# 编译选项
CFLAGS = -Wall -Wextra -I./include

# 源文件目录
SRC_DIR = src
APP_DIR = app
MONITOR_DIR = monitor

# 公共源文件
COMMON_SRC = $(SRC_DIR)/shm.c $(SRC_DIR)/sem.c $(SRC_DIR)/msgq.c $(SRC_DIR)/log.c

# 监控模块源文件
MONITOR_SRC = $(MONITOR_DIR)/memory.c $(MONITOR_DIR)/temp.c $(MONITOR_DIR)/cpu.c $(MONITOR_DIR)/disk.c $(MONITOR_DIR)/network_stat.c

# TCP / 配置 / 协议文件
TCP_SRC = $(SRC_DIR)/tcp_client.c
CONFIG_SRC = $(SRC_DIR)/config.c
PROTOCOL_SRC = $(SRC_DIR)/protocol.c

# 目标文件
MANAGER = manager
COLLECTOR = collector
NETWORK = network
LOGGER = logger
WEB = web

# 默认目标
all: $(MANAGER) $(COLLECTOR) $(NETWORK) $(LOGGER) $(WEB)

# 编译 manager
$(MANAGER): $(APP_DIR)/manager.c $(COMMON_SRC) $(CONFIG_SRC)
	$(CC) $(CFLAGS) -o $@ $^ -lrt -pthread

# 编译 collector
$(COLLECTOR): $(APP_DIR)/collector.c $(COMMON_SRC) $(MONITOR_SRC) $(CONFIG_SRC)
	$(CC) $(CFLAGS) -o $@ $^ -lrt -pthread

# 编译 network
$(NETWORK): $(APP_DIR)/network.c $(COMMON_SRC) $(CONFIG_SRC) $(TCP_SRC) $(PROTOCOL_SRC)
	$(CC) $(CFLAGS) -o $@ $^ -lrt -pthread

# 编译 logger
$(LOGGER): $(APP_DIR)/logger.c $(COMMON_SRC)
	$(CC) $(CFLAGS) -o $@ $^ -lrt -pthread

# 编译 web
$(WEB): $(APP_DIR)/web.c $(COMMON_SRC)
	$(CC) $(CFLAGS) -o $@ $^ -lrt -pthread

# 编译测试程序（只链接需要的 monitor 模块）
TEST_MEMORY = test_memory
TEST_TEMP   = test_temp
TEST_CPU    = test_cpu
TEST_MSGQ   = test_msgq
TEST_TCP_CLIENT = test_tcp_client
TEST_TCP_SERVER = test_tcp_server
TEST_PROTOCOL = test_protocol
TEST_CONFIG  = test_config

$(TEST_MEMORY): test/test_memory.c monitor/memory.c
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_TEMP): test/test_temp.c monitor/temp.c
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_CPU): test/test_cpu.c monitor/cpu.c  
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_MSGQ): test/test_msgq.c src/msgq.c
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_TCP_CLIENT): test/test_tcp_client.c $(TCP_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_TCP_SERVER): test/test_tcp_server.c
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_PROTOCOL): test/test_protocol.c src/protocol.c
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_CONFIG): test/test_config.c src/config.c
	$(CC) $(CFLAGS) -o $@ $^

# 一键编译所有测试
tests: $(TEST_MEMORY) $(TEST_TEMP) $(TEST_CPU) $(TEST_MSGQ) $(TEST_TCP_CLIENT) $(TEST_TCP_SERVER) $(TEST_PROTOCOL) $(TEST_CONFIG)

# 清理
clean:
	rm -f $(MANAGER) $(COLLECTOR) $(NETWORK) $(LOGGER) $(WEB) $(TEST_MEMORY) $(TEST_TEMP) $(TEST_CPU) $(TEST_MSGQ) $(TEST_TCP_CLIENT) $(TEST_TCP_SERVER) $(TEST_PROTOCOL) $(TEST_CONFIG)
