#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "msgq.h"

/*
 * 功能：
 *   生成 System V 消息队列使用的 key
 *
 * 说明：
 *   ftok() 需要一个真实存在的文件路径
 *   所以这里先用 open() 创建一个临时 key 文件
 */
static key_t msgq_make_key(void)
{
    int fd;
    key_t key;

    /*
     * 创建 key 文件
     * 如果文件已经存在，open 不会清空它
     */
    fd = open(MSGQ_KEY_PATH, O_CREAT | O_RDWR, 0666);
    if (fd == -1)
    {
        perror("open msgq key file failed");
        return -1;
    }

    close(fd);

    /*
     * 使用 ftok 根据路径和项目 ID 生成 key
     */
    key = ftok(MSGQ_KEY_PATH, MSGQ_PROJ_ID);
    if (key == -1)
    {
        perror("ftok failed");
        return -1;
    }

    return key;
}

/*
 * 功能：
 *   创建或打开消息队列
 */
int msgq_create(void)
{
    key_t key;
    int msgid;

    key = msgq_make_key();
    if (key == -1)
    {
        return -1;
    }

    /*
     * msgget 创建或打开消息队列
     *
     * IPC_CREAT：
     *   如果消息队列不存在，就创建
     *
     * 0666：
     *   设置权限
     */
    msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1)
    {
        perror("msgget create failed");
        return -1;
    }

    return msgid;
}

/*
 * 功能：
 *   打开已经存在的消息队列
 */
int msgq_open_existing(void)
{
    key_t key;
    int msgid;

    key = msgq_make_key();
    if (key == -1)
    {
        return -1;
    }

    /*
     * 不带 IPC_CREAT，表示只打开已有队列
     */
    msgid = msgget(key, 0666);
    if (msgid == -1)
    {
        perror("msgget open existing failed");
        return -1;
    }

    return msgid;
}

/*
 * 功能：
 *   发送消息
 */
int msgq_send(int msgid, long type, const char *text)
{
    msgq_msg_t msg;

    if (msgid < 0 || type <= 0 || text == NULL)
    {
        return -1;
    }

    /*
     * 清空结构体，避免残留数据
     */
    memset(&msg, 0, sizeof(msg));

    /*
     * 设置消息类型
     * System V 消息队列要求 msg_type 必须大于 0
     */
    msg.msg_type = type;

    /*
     * 拷贝消息内容
     * strncpy 防止字符串超过缓冲区
     */
    strncpy(msg.msg_text, text, MSG_TEXT_SIZE - 1);
    msg.msg_text[MSG_TEXT_SIZE - 1] = '\0';

    /*
     * msgsnd 第三个参数是消息正文大小
     * 注意：
     *   不包括 long msg_type
     */
    if (msgsnd(msgid, &msg, sizeof(msg.msg_text), 0) == -1)
    {
        perror("msgsnd failed");
        return -1;
    }

    return 0;
}

/*
 * 功能：
 *   接收消息
 */
int msgq_recv(int msgid, long type, msgq_msg_t *out)
{
    if (msgid < 0 || out == NULL)
    {
        return -1;
    }

    memset(out, 0, sizeof(msgq_msg_t));

    /*
     * msgrcv 第三个参数是消息正文大小
     * 注意：
     *   不包括 long msg_type
     *
     * type = 0：
     *   接收队列中第一条消息
     *
     * type > 0：
     *   接收指定类型的第一条消息
     */
    if (msgrcv(msgid, out, sizeof(out->msg_text), type, 0) == -1)
    {
        perror("msgrcv failed");
        return -1;
    }

    return 0;
}

/*
 * 功能：
 *   删除消息队列
 */
int msgq_remove(int msgid)
{
    if (msgid < 0)
    {
        return -1;
    }

    /*
     * IPC_RMID 表示删除消息队列
     */
    if (msgctl(msgid, IPC_RMID, NULL) == -1)
    {
        perror("msgctl IPC_RMID failed");
        return -1;
    }

    return 0;
}