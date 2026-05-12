#ifndef LOG_H
#define LOG_H

/*
 * 日志等级定义
 * 后面可以根据等级做不同处理
 */
#define LOG_TYPE_INFO   1
#define LOG_TYPE_WARN   2
#define LOG_TYPE_ERROR  3
#define LOG_TYPE_EXIT   9

/*
 * 功能：
 *   初始化日志模块
 *
 * 说明：
 *   实际上就是打开已经存在的消息队列
 *   collector / network / manager 调用后，才能发送日志
 *
 * 返回值：
 *   成功：0
 *   失败：-1
 */
int log_init(void);

/*
 * 功能：
 *   发送普通日志
 */
int log_info(const char *fmt, ...);

/*
 * 功能：
 *   发送警告日志
 */
int log_warn(const char *fmt, ...);

/*
 * 功能：
 *   发送错误日志
 */
int log_error(const char *fmt, ...);

/*
 * 功能：
 *   通知 logger 进程退出
 */
int log_send_exit(void);

#endif