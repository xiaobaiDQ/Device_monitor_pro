#ifndef CONFIG_H
#define CONFIG_H

/*
 * 配置文件默认路径
 */
#define CONFIG_FILE_PATH "config/monitor.conf"

/*
 * 配置结构体
 *
 * 这个结构体用于保存从 monitor.conf 中读取出来的配置项
 */
typedef struct
{
    int run_time;              /* manager 运行时间，单位：秒 */
    int sample_interval;       /* collector 采集周期，单位：秒 */

    float cpu_threshold;       /* CPU 告警阈值 */
    float mem_threshold;       /* 内存告警阈值 */
    float temp_acpitz_1_threshold;      /* 温度告警阈值 */
    float temp_acpitz_2_threshold;      /* 温度告警阈值 */
    float temp_cpu_threshold;      /* 温度告警阈值 */

    char server_ip[32];        /* TCP 服务器 IP */
    int server_port;           /* TCP 服务器端口 */

    char network_iface[32];    /* 网络接口名 */

    char log_file[128];        /* 日志文件路径 */
} monitor_config_t;

/*
 * 功能：
 *   加载配置文件
 *
 * 参数：
 *   path：配置文件路径
 *   cfg ：用于保存解析结果的结构体指针
 *
 * 返回值：
 *   成功：0
 *   失败：-1
 */
int config_load(const char *path, monitor_config_t *cfg);

/*
 * 功能：
 *   给配置结构体填充默认值
 *
 * 作用：
 *   即使配置文件中缺少某些字段，程序也可以使用默认值运行
 */
void config_set_default(monitor_config_t *cfg);

/*
 * 功能：
 *   打印当前配置
 *
 * 主要用于测试和调试
 */
void config_print(const monitor_config_t *cfg);

#endif