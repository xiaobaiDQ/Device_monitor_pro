#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "common.h"

/*
 * 命令类型枚举
 *
 * 服务端发送过来的字符串命令，最终会被解析成这些类型。
 */
typedef enum
{
    CMD_UNKNOWN = 0,       /* 未知命令 */
    CMD_GET_STATUS,        /* 获取设备状态 */
    CMD_SET_INTERVAL,      /* 设置上传周期 */
    CMD_STOP               /* 停止系统 */
} command_type_t;

/*
 * 命令结构体
 *
 * 解析服务端命令后，将结果保存到这个结构体中。
 */
typedef struct
{
    command_type_t type;   /* 命令类型 */
    int interval;          /* SET_INTERVAL 命令携带的上传周期 */
    char raw[128];         /* 原始命令字符串，便于日志记录 */
} command_t;

/*
 * 功能：
 *   构造设备状态 JSON 字符串
 *
 * 示例输出：
 *   {"type":"status","cpu":10.5,"mem":42.1,"temp":50.0}
 *
 * 参数：
 *   status：当前设备状态
 *   buf   ：输出缓冲区
 *   size  ：缓冲区大小
 *
 * 返回值：
 *   成功：0
 *   失败：-1
 */
int protocol_build_status_json(const system_status_t *status,
                               char *buf,
                               int size);

/*
 * 功能：
 *   构造 GET_STATUS 命令的 ACK 回复
 *
 * 示例输出：
 *   {"type":"ack","cmd":"GET_STATUS","cpu":10.5,"mem":42.1,"temp":50.0}
 */
int protocol_build_ack_status(const system_status_t *status,
                              char *buf,
                              int size);

/*
 * 功能：
 *   构造 SET_INTERVAL 命令的 ACK 回复
 *
 * 示例输出：
 *   {"type":"ack","cmd":"SET_INTERVAL","interval":5}
 */
int protocol_build_ack_interval(int interval,
                                char *buf,
                                int size);

/*
 * 功能：
 *   构造 STOP 命令的 ACK 回复
 *
 * 示例输出：
 *   {"type":"ack","cmd":"STOP","msg":"system stopping"}
 */
int protocol_build_ack_stop(char *buf, int size);

/*
 * 功能：
 *   构造错误回复
 *
 * 示例输出：
 *   {"type":"error","msg":"unknown command"}
 */
int protocol_build_error(const char *msg,
                         char *buf,
                         int size);

/*
 * 功能：
 *   解析服务端命令
 *
 * 支持：
 *   GET_STATUS
 *   SET_INTERVAL 2
 *   STOP
 *
 * 参数：
 *   cmd_str：服务端发送过来的命令字符串
 *   cmd    ：解析结果输出
 *
 * 返回值：
 *   成功：0
 *   失败：-1
 */
int protocol_parse_command(const char *cmd_str, command_t *cmd);

#endif