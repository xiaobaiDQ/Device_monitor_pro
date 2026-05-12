#include <stdio.h>
#include "msgq.h"

int main(void)
{
    int msgid;
    msgq_msg_t msg;

    /*
     * 创建消息队列
     */
    msgid = msgq_create();
    if (msgid == -1)
    {
        printf("msgq_create failed\n");
        return 1;
    }

    printf("msgq_create success, msgid = %d\n", msgid);

    /*
     * 发送一条消息
     *
     * 消息类型设置为 1
     */
    if (msgq_send(msgid, 1, "hello msg queue, this is a log message") != 0)
    {
        printf("msgq_send failed\n");
        msgq_remove(msgid);
        return 1;
    }

    printf("msgq_send success\n");

    /*
     * 接收消息
     *
     * type = 0 表示接收队列中的第一条消息
     */
    if (msgq_recv(msgid, 0, &msg) != 0)
    {
        printf("msgq_recv failed\n");
        msgq_remove(msgid);
        return 1;
    }

    printf("msgq_recv success\n");
    printf("msg_type = %ld\n", msg.msg_type);
    printf("msg_text = %s\n", msg.msg_text);

    /*
     * 删除消息队列
     */
    if (msgq_remove(msgid) != 0)
    {
        printf("msgq_remove failed\n");
        return 1;
    }

    printf("msgq_remove success\n");

    return 0;
}