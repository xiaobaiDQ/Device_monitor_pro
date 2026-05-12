#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "log.h"
#include "msgq.h"

/*
 * 保存当前进程打开的消息队列 ID
 * static 表示只在本文件内部使用
 */
static int g_log_msgid = -1;

/*
 * 功能：
 *   把日志等级转换成字符串
 */
static const char *log_level_to_string(long type)
{
    switch (type)
    {
    case LOG_TYPE_INFO:
        return "INFO";
    case LOG_TYPE_WARN:
        return "WARN";
    case LOG_TYPE_ERROR:
        return "ERROR";
    case LOG_TYPE_EXIT:
        return "EXIT";
    default:
        return "UNKNOWN";
    }
}

/*
 * 功能：
 *   生成当前时间字符串
 *
 * 输出格式示例：
 *   2026-04-26 12:30:15
 */
static void get_time_string(char *buf, int size)
{
    time_t now;
    struct tm *tm_info;

    if (buf == NULL || size <= 0)
    {
        return;
    }

    now = time(NULL);
    tm_info = localtime(&now);

    if (tm_info == NULL)
    {
        snprintf(buf, size, "unknown-time");
        return;
    }

    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/*
 * 功能：
 *   初始化日志模块
 *
 * 注意：
 *   这里是打开已有消息队列，不是创建消息队列
 *   因为消息队列应该由 manager 创建
 */
int log_init(void)
{
    g_log_msgid = msgq_open_existing();

    if (g_log_msgid == -1)
    {
        return -1;
    }

    return 0;
}

/*
 * 功能：
 *   日志发送核心函数
 *
 * 参数：
 *   type：日志类型
 *   fmt ：格式化字符串
 */
static int log_send(long type, const char *fmt, va_list ap)
{
    /*
     * user_msg 用来保存用户传入的日志内容
     *
     * 注意：
     *   这里不能直接用 MSG_TEXT_SIZE，
     *   因为最终日志还要加上时间戳和日志等级。
     *
     *   例如：
     *   [2026-04-26 13:31:06] [INFO] 用户日志内容
     *
     *   所以用户日志内容要预留一部分空间。
     */
    char user_msg[160];

    /*
     * final_msg 是最终发给消息队列的完整日志
     *
     * 它的大小不能超过 MSG_TEXT_SIZE，
     * 因为 msgq_send() 最终会把它拷贝到 msg_text[MSG_TEXT_SIZE]。
     */
    char final_msg[MSG_TEXT_SIZE];

    /*
     * 保存时间字符串
     */
    char time_buf[64];

    int ret;

    if (g_log_msgid == -1 || fmt == NULL)
    {
        return -1;
    }

    memset(user_msg, 0, sizeof(user_msg));
    memset(final_msg, 0, sizeof(final_msg));
    memset(time_buf, 0, sizeof(time_buf));

    /*
     * 把用户传进来的格式化日志先写入 user_msg
     *
     * 由于 user_msg 只有 160 字节，
     * 即使用户传入很长的日志，也会被安全截断，
     * 不会造成 final_msg 拼接时溢出。
     */
    vsnprintf(user_msg, sizeof(user_msg), fmt, ap);

    /*
     * 获取当前时间字符串
     */
    get_time_string(time_buf, sizeof(time_buf));

    /*
     * 拼接最终日志
     *
     * 这里使用 snprintf 是安全的，
     * 即使最终内容超过 final_msg 大小，也只会被截断，
     * 不会发生缓冲区溢出。
     */
    ret = snprintf(final_msg,
                   sizeof(final_msg),
                   "[%s] [%s] %s",
                   time_buf,
                   log_level_to_string(type),
                   user_msg);

    /*
     * ret < 0 表示 snprintf 失败
     */
    if (ret < 0)
    {
        return -1;
    }

    /*
     * 发送到消息队列
     */
    return msgq_send(g_log_msgid, type, final_msg);
}

/*
 * 功能：
 *   发送 INFO 日志
 */
int log_info(const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = log_send(LOG_TYPE_INFO, fmt, ap);
    va_end(ap);

    return ret;
}

/*
 * 功能：
 *   发送 WARN 日志
 */
int log_warn(const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = log_send(LOG_TYPE_WARN, fmt, ap);
    va_end(ap);

    return ret;
}

/*
 * 功能：
 *   发送 ERROR 日志
 */
int log_error(const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = log_send(LOG_TYPE_ERROR, fmt, ap);
    va_end(ap);

    return ret;
}

/*
 * 功能：
 *   给 logger 发送退出消息
 */
int log_send_exit(void)
{
    char final_msg[MSG_TEXT_SIZE];
    char time_buf[64];

    if (g_log_msgid == -1)
    {
        return -1;
    }

    memset(final_msg, 0, sizeof(final_msg));
    memset(time_buf, 0, sizeof(time_buf));

    /*
     * 获取当前时间字符串
     */
    get_time_string(time_buf, sizeof(time_buf));

    /*
     * 拼接退出日志
     */
    snprintf(final_msg,
             sizeof(final_msg),
             "[%s] [EXIT] logger exit",
             time_buf);

    /*
     * 发送退出消息
     */
    return msgq_send(g_log_msgid, LOG_TYPE_EXIT, final_msg);
}