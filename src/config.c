#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "config.h"

/*
 * 功能：
 *   去除字符串左侧和右侧的空白字符
 *
 * 例如：
 *   "  run_time  " -> "run_time"
 *
 * 返回值：
 *   返回处理后的字符串起始地址
 */
static char *trim(char *str)
{
    char *end;

    if (str == NULL)
    {
        return NULL;
    }

    /*
     * 跳过左侧空白字符
     */
    while (isspace((unsigned char)*str))
    {
        str++;
    }

    /*
     * 如果整行都是空白，直接返回
     */
    if (*str == '\0')
    {
        return str;
    }

    /*
     * 找到字符串末尾
     */
    end = str + strlen(str) - 1;

    /*
     * 去掉右侧空白字符
     */
    while (end > str && isspace((unsigned char)*end))
    {
        *end = '\0';
        end--;
    }

    return str;
}

/*
 * 功能：
 *   设置默认配置
 *
 * 说明：
 *   这样即使配置文件读取失败或者缺少字段，
 *   程序也有一套默认参数可用。
 */
void config_set_default(monitor_config_t *cfg)
{
    if (cfg == NULL)
    {
        return;
    }

    cfg->run_time = 10;
    cfg->sample_interval = 1;

    cfg->cpu_threshold = 80.0f;
    cfg->mem_threshold = 85.0f;
    cfg->temp_acpitz_1_threshold = 70.0f;
    cfg->temp_acpitz_2_threshold = 70.0f;
    cfg->temp_cpu_threshold = 70.0f;
    
    snprintf(cfg->server_ip, sizeof(cfg->server_ip), "127.0.0.1");
    cfg->server_port = 8888;
    
    snprintf(cfg->network_iface, sizeof(cfg->network_iface), "enp3s0");
    
    snprintf(cfg->log_file, sizeof(cfg->log_file), "logs/monitor.log");
}

/*
 * 功能：
 *   根据 key-value 更新配置结构体
 */
static void config_set_value(monitor_config_t *cfg, const char *key, const char *value)
{
    if (cfg == NULL || key == NULL || value == NULL)
    {
        return;
    }

    /*
     * strcmp 返回 0 表示字符串相等
     */
    if (strcmp(key, "run_time") == 0)
    {
        cfg->run_time = atoi(value);
    }
    else if (strcmp(key, "sample_interval") == 0)
    {
        cfg->sample_interval = atoi(value);
    }
    else if (strcmp(key, "cpu_threshold") == 0)
    {
        cfg->cpu_threshold = (float)atof(value);
    }
    else if (strcmp(key, "mem_threshold") == 0)
    {
        cfg->mem_threshold = (float)atof(value);
    }
    else if (strcmp(key, "temp_acpitz_1_threshold") == 0)
    {
        cfg->temp_acpitz_1_threshold = (float)atof(value);
    }
    else if (strcmp(key, "temp_acpitz_2_threshold") == 0)
    {
        cfg->temp_acpitz_2_threshold = (float)atof(value);
    }
    else if (strcmp(key, "temp_cpu_threshold") == 0)
    {
        cfg->temp_cpu_threshold = (float)atof(value);
    }
    else if (strcmp(key, "server_ip") == 0)
    {
        snprintf(cfg->server_ip, sizeof(cfg->server_ip), "%s", value);
    }
    else if (strcmp(key, "server_port") == 0)
    {
        cfg->server_port = atoi(value);
    }else if (strcmp(key, "network_iface") == 0)
    {
        snprintf(cfg->network_iface, sizeof(cfg->network_iface), "%s", value);
    }
    else if (strcmp(key, "log_file") == 0)
    {
        /*
         * 安全拷贝日志文件路径
         */
        snprintf(cfg->log_file, sizeof(cfg->log_file), "%s", value);
    }
}

/*
 * 功能：
 *   加载配置文件
 *
 * 配置格式：
 *   key=value
 *
 * 支持：
 *   空行
 *   # 开头的注释
 */
int config_load(const char *path, monitor_config_t *cfg)
{
    FILE *fp = NULL;
    char line[256];

    if (path == NULL || cfg == NULL)
    {
        return -1;
    }

    /*
     * 先设置默认值
     * 后面如果配置文件里有对应字段，就覆盖默认值
     */
    config_set_default(cfg);

    fp = fopen(path, "r");
    if (fp == NULL)
    {
        perror("fopen config file failed");
        return -1;
    }

    /*
     * 一行一行读取配置文件
     */
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        char *p;
        char *equal_pos;
        char *key;
        char *value;

        /*
         * 去掉行首行尾空白
         */
        p = trim(line);

        /*
         * 跳过空行
         */
        if (*p == '\0')
        {
            continue;
        }

        /*
         * 跳过注释行
         */
        if (*p == '#')
        {
            continue;
        }

        /*
         * 查找等号
         */
        equal_pos = strchr(p, '=');
        if (equal_pos == NULL)
        {
            continue;
        }

        /*
         * 把 key=value 切成两个字符串
         */
        *equal_pos = '\0';

        key = trim(p);
        value = trim(equal_pos + 1);

        /*
         * 根据 key 设置配置值
         */
        config_set_value(cfg, key, value);
    }

    fclose(fp);

    /*
     * 简单保护：防止配置成 0 或负数导致程序异常
     */
    if (cfg->run_time <= 0)
    {
        cfg->run_time = 10;
    }

    if (cfg->sample_interval <= 0)
    {
        cfg->sample_interval = 1;
    }

    if (cfg->server_port <= 0)
    {
        cfg->server_port = 8888;
    }

    return 0;
}

/*
 * 功能：
 *   打印配置内容
 */
void config_print(const monitor_config_t *cfg)
{
    if (cfg == NULL)
    {
        return;
    }

    printf("========== monitor config ==========\n");
    printf("run_time        = %d\n", cfg->run_time);
    printf("sample_interval = %d\n", cfg->sample_interval);
    printf("cpu_threshold   = %.1f\n", cfg->cpu_threshold);
    printf("mem_threshold   = %.1f\n", cfg->mem_threshold);
    printf("temp_acpitz_1_threshold  = %.1f\n", cfg->temp_acpitz_1_threshold);
    printf("temp_acpitz_2_threshold  = %.1f\n", cfg->temp_acpitz_2_threshold);
    printf("temp_cpu_threshold  = %.1f\n", cfg->temp_cpu_threshold);
    printf("server_ip       = %s\n", cfg->server_ip);
    printf("server_port     = %d\n", cfg->server_port);
    printf("log_file        = %s\n", cfg->log_file);
    printf("====================================\n");
}