#ifndef MSGQ_H
#define MSGQ_H

#include <sys/types.h>

/*
 * 消息队列 key 文件路径
 *
 * ftok() 会用这个路径和一个项目 ID 生成 key_t
 * 注意：
 *   这个文件必须存在
 *   所以后面 msgq_create() 里会自动创建它
 */
#define MSGQ_KEY_PATH "/tmp/device_monitor_msgq"

/*
 * ftok 使用的项目 ID
 * 可以理解为生成 key 的辅助编号
 */
#define MSGQ_PROJ_ID  'M'

/*
 * 单条日志消息最大长度
 */
#define MSG_TEXT_SIZE 256

/*
 * 消息结构体
 *
 * 注意：
 *   System V 消息队列要求第一个成员必须是 long 类型
 *   这个字段表示消息类型
 */
typedef struct
{
    long msg_type;                    /* 消息类型，必须大于 0 */
    char msg_text[MSG_TEXT_SIZE];     /* 消息内容 */
} msgq_msg_t;

/*
 * 功能：
 *   创建或打开消息队列
 *
 * 返回值：
 *   成功：返回消息队列 ID
 *   失败：返回 -1
 */
int msgq_create(void);

/*
 * 功能：
 *   打开已经存在的消息队列
 *
 * 返回值：
 *   成功：返回消息队列 ID
 *   失败：返回 -1
 */
int msgq_open_existing(void);

/*
 * 功能：
 *   发送消息到消息队列
 *
 * 参数：
 *   msgid：消息队列 ID
 *   type ：消息类型，必须大于 0
 *   text ：消息内容字符串
 *
 * 返回值：
 *   成功：0
 *   失败：-1
 */
int msgq_send(int msgid, long type, const char *text);

/*
 * 功能：
 *   从消息队列接收消息
 *
 * 参数：
 *   msgid：消息队列 ID
 *   type ：要接收的消息类型
 *          如果 type = 0，表示接收队列中的第一条消息
 *   out  ：用于保存接收到的消息
 *
 * 返回值：
 *   成功：0
 *   失败：-1
 */
int msgq_recv(int msgid, long type, msgq_msg_t *out);

/*
 * 功能：
 *   删除消息队列
 *
 * 参数：
 *   msgid：消息队列 ID
 *
 * 返回值：
 *   成功：0
 *   失败：-1
 */
int msgq_remove(int msgid);

#endif