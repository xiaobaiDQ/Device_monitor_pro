#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "protocol.h"

/*
 * 功能：
 *   去掉字符串右侧的换行符和空白字符
 *
 * 例如：
 *   "GET_STATUS\n" -> "GET_STATUS"
 */
static void trim_right(char *str)
{
    int len;

    if (str == NULL)
    {
        return;
    }

    len = strlen(str);

    while (len > 0)
    {
        if (str[len - 1] == '\n' ||
            str[len - 1] == '\r' ||
            isspace((unsigned char)str[len - 1]))
        {
            str[len - 1] = '\0';
            len--;
        }
        else
        {
            break;
        }
    }
}

/*
 * 功能：
 *   去掉字符串左侧空白字符
 *
 * 返回值：
 *   返回第一个非空白字符的位置
 */
static char *trim_left(char *str)
{
    if (str == NULL)
    {
        return NULL;
    }

    while (*str != '\0' && isspace((unsigned char)*str))
    {
        str++;
    }

    return str;
}

/*
 * 功能：
 *   构造设备状态 JSON 字符串
 */
int protocol_build_status_json(const system_status_t *status,
                               char *buf,
                               int size)
{
    int ret;

    if (status == NULL || buf == NULL || size <= 0)
    {
        return -1;
    }

    /*
     * 构造统一 JSON 格式
     *
     * 末尾加 \n 是为了服务端打印时更清晰，
     * 也方便后面按行解析数据。
     */
    ret = snprintf(buf,
                   size,
                   "{\"type\":\"status\",\"cpu\":%.1f,\"mem\":%.1f,\"temp_acpitz_1\":%.1f,\"temp_acpitz_2\":%.1f,\"temp_cpu\":%.1f,\"disk\":%.1f,\"rx_kbps\":%.1f,\"tx_kbps\":%.1f}\n",
                   status->cpu_usage,
                   status->mem_usage,
                   status->temp_acpitz_1,
                   status->temp_acpitz_2,
                   status->temp_cpu,
                   status->disk_usage,
                   status->rx_kbps,
                   status->tx_kbps);

    if (ret < 0 || ret >= size)
    {
        return -1;
    }

    return 0;
}

/*
 * 功能：
 *   构造 GET_STATUS 的 ACK 回复
 */
int protocol_build_ack_status(const system_status_t *status,
                              char *buf,
                              int size)
{
    int ret;

    if (status == NULL || buf == NULL || size <= 0)
    {
        return -1;
    }

    ret = snprintf(buf,
                   size,
                   "{\"type\":\"ack\",\"cmd\":\"GET_STATUS\",\"cpu\":%.1f,\"mem\":%.1f,\"temp_acpitz_1\":%.1f,\"temp_acpitz_2\":%.1f,\"temp_cpu\":%.1f,\"disk\":%.1f,\"rx_kbps\":%.1f,\"tx_kbps\":%.1f}\n",
                   status->cpu_usage,
                   status->mem_usage,
                   status->temp_acpitz_1,
                   status->temp_acpitz_2,
                   status->temp_cpu,
                   status->disk_usage,
                   status->rx_kbps,
                   status->tx_kbps);

    if (ret < 0 || ret >= size)
    {
        return -1;
    }

    return 0;
}

/*
 * 功能：
 *   构造 SET_INTERVAL 的 ACK 回复
 */
int protocol_build_ack_interval(int interval,
                                char *buf,
                                int size)
{
    int ret;

    if (buf == NULL || size <= 0)
    {
        return -1;
    }

    ret = snprintf(buf,
                   size,
                   "{\"type\":\"ack\",\"cmd\":\"SET_INTERVAL\",\"interval\":%d}\n",
                   interval);

    if (ret < 0 || ret >= size)
    {
        return -1;
    }

    return 0;
}

/*
 * 功能：
 *   构造 STOP 的 ACK 回复
 */
int protocol_build_ack_stop(char *buf, int size)
{
    int ret;

    if (buf == NULL || size <= 0)
    {
        return -1;
    }

    ret = snprintf(buf,
                   size,
                   "{\"type\":\"ack\",\"cmd\":\"STOP\",\"msg\":\"system stopping\"}\n");

    if (ret < 0 || ret >= size)
    {
        return -1;
    }

    return 0;
}

/*
 * 功能：
 *   构造错误回复
 */
int protocol_build_error(const char *msg,
                         char *buf,
                         int size)
{
    int ret;

    if (msg == NULL || buf == NULL || size <= 0)
    {
        return -1;
    }

    ret = snprintf(buf,
                   size,
                   "{\"type\":\"error\",\"msg\":\"%s\"}\n",
                   msg);

    if (ret < 0 || ret >= size)
    {
        return -1;
    }

    return 0;
}

/*
 * 功能：
 *   解析服务端命令
 */
int protocol_parse_command(const char *cmd_str, command_t *cmd)
{
    char tmp[128];
    char *p;

    if (cmd_str == NULL || cmd == NULL)
    {
        return -1;
    }

    /*
     * 先清空输出结构体
     */
    memset(cmd, 0, sizeof(command_t));
    cmd->type = CMD_UNKNOWN;
    cmd->interval = 0;

    /*
     * 拷贝一份命令字符串，避免修改原始输入
     */
    snprintf(tmp, sizeof(tmp), "%s", cmd_str);

    /*
     * 去掉右侧换行和空白
     */
    trim_right(tmp);

    /*
     * 去掉左侧空白
     */
    p = trim_left(tmp);

    if (p == NULL || *p == '\0')
    {
        cmd->type = CMD_UNKNOWN;
        snprintf(cmd->raw, sizeof(cmd->raw), "");
        return 0;
    }

    /*
     * 保存原始命令
     */
    snprintf(cmd->raw, sizeof(cmd->raw), "%s", p);

    /*
     * 解析 GET_STATUS
     */
    if (strcmp(p, "GET_STATUS") == 0)
    {
        cmd->type = CMD_GET_STATUS;
        return 0;
    }

    /*
     * 解析 STOP
     */
    if (strcmp(p, "STOP") == 0)
    {
        cmd->type = CMD_STOP;
        return 0;
    }

    /*
     * 解析 SET_INTERVAL
     *
     * 合法格式：
     *   SET_INTERVAL 2
     */
    if (strncmp(p, "SET_INTERVAL", 12) == 0)
    {
        int interval = 0;

        if (sscanf(p, "SET_INTERVAL %d", &interval) == 1)
        {
            cmd->type = CMD_SET_INTERVAL;
            cmd->interval = interval;
            return 0;
        }

        /*
         * 如果格式不对，仍然归类为 SET_INTERVAL，
         * 但是 interval 保持 0，方便上层判断错误。
         */
        cmd->type = CMD_SET_INTERVAL;
        cmd->interval = 0;
        return 0;
    }

    /*
     * 其他情况全部视为未知命令
     */
    cmd->type = CMD_UNKNOWN;
    return 0;
}