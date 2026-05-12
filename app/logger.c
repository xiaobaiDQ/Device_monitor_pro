#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "msgq.h"
#include "log.h"

#define LOG_FILE_PATH "logs/monitor.log"

int main(void)
{
    int msgid;
    FILE *fp = NULL;
    msgq_msg_t msg;

    /*
     * 打开已有消息队列
     * 注意：
     *   消息队列由 manager 创建
     *   logger 这里只负责打开和读取
     */
    msgid = msgq_open_existing();
    if (msgid == -1)
    {
        printf("[logger] msgq_open_existing failed\n");
        return 1;
    }

    /*
     * 以追加方式打开日志文件
     * 如果文件不存在，会自动创建
     */
    fp = fopen(LOG_FILE_PATH, "a");
    if (fp == NULL)
    {
        perror("[logger] fopen log file failed");
        return 1;
    }

    printf("[logger] start, writing log to %s\n", LOG_FILE_PATH);

    /*
     * 循环接收日志消息
     */
    while (1)
    {
        memset(&msg, 0, sizeof(msg));

        /*
         * type = 0 表示接收队列中的第一条消息
         * 这里会阻塞等待日志
         */
        if (msgq_recv(msgid, 0, &msg) != 0)
        {
            printf("[logger] msgq_recv failed\n");
            break;
        }

        /*
         * 收到退出消息
         */
        if (msg.msg_type == LOG_TYPE_EXIT)
        {
            fprintf(fp, "%s\n", msg.msg_text);
            fflush(fp);

            printf("[logger] receive exit message\n");
            break;
        }

        /*
         * 写入日志文件
         */
        fprintf(fp, "%s\n", msg.msg_text);

        /*
         * 立即刷新，防止程序异常退出时日志丢失
         * 后面如果追求性能，可以改成定期刷新
         */
        fflush(fp);
    }

    fclose(fp);

    printf("[logger] exit\n");

    return 0;
}