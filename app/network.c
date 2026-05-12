#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "shm.h"
#include "sem.h"
#include "log.h"
#include "config.h"
#include "tcp_client.h"
#include "protocol.h"

#define CMD_BUF_SIZE 256
#define SEND_BUF_SIZE 256



/*
 * 功能：
 *   处理服务器下发的命令，并给服务端回复执行结果
 *
 * 支持命令：
 *   GET_STATUS
 *   SET_INTERVAL 2
 *   STOP
 */
/*
 * 功能：
 *   处理服务器下发的命令，并给服务端回复执行结果
 */
static void handle_server_command(const char *cmd_str,
                                  system_status_t *status,
                                  sem_t *sem,
                                  int *upload_interval,
                                  int sockfd)
{
    command_t cmd;
    char reply_buf[256];
    system_status_t current_status;

    if (cmd_str == NULL || status == NULL || sem == NULL || upload_interval == NULL)
    {
        return;
    }

    memset(reply_buf, 0, sizeof(reply_buf));

    /*
     * 解析服务端命令
     */
    if (protocol_parse_command(cmd_str, &cmd) != 0)
    {
        protocol_build_error("parse command failed", reply_buf, sizeof(reply_buf));
        tcp_client_send(sockfd, reply_buf);
        log_warn("[network] parse command failed");
        return;
    }

    printf("[network] receive command: %s\n", cmd.raw);
    log_info("[network] receive command: %s", cmd.raw);

    /*
     * 根据命令类型分别处理
     */
    switch (cmd.type)
    {
    case CMD_GET_STATUS:
        /*
         * 读取当前共享内存中的状态
         */
        sem_lock(sem);

        current_status = *status;

        sem_unlock(sem);

        /*
         * 构造 ACK 状态回复
         */
        if (protocol_build_ack_status(&current_status,
                                      reply_buf,
                                      sizeof(reply_buf)) == 0)
        {
            tcp_client_send(sockfd, reply_buf);
            log_info("[network] GET_STATUS reply sent");
        }
        else
        {
            protocol_build_error("build GET_STATUS ack failed",
                                 reply_buf,
                                 sizeof(reply_buf));
            tcp_client_send(sockfd, reply_buf);
            log_warn("[network] build GET_STATUS ack failed");
        }
        break;

    case CMD_SET_INTERVAL:
        /*
         * interval <= 0 表示格式错误或非法值
         */
        if (cmd.interval > 0 && cmd.interval <= 60)
        {
            *upload_interval = cmd.interval;

            if (protocol_build_ack_interval(*upload_interval,
                                            reply_buf,
                                            sizeof(reply_buf)) == 0)
            {
                tcp_client_send(sockfd, reply_buf);
            }

            printf("[network] upload interval set to %d\n", *upload_interval);
            log_info("[network] upload interval set to %d", *upload_interval);
        }
        else
        {
            protocol_build_error("invalid interval",
                                 reply_buf,
                                 sizeof(reply_buf));
            tcp_client_send(sockfd, reply_buf);

            printf("[network] invalid interval: %d\n", cmd.interval);
            log_warn("[network] invalid interval: %d", cmd.interval);
        }
        break;

    case CMD_STOP:
        /*
         * 先回复 ACK，再通知系统停止
         */
        if (protocol_build_ack_stop(reply_buf, sizeof(reply_buf)) == 0)
        {
            tcp_client_send(sockfd, reply_buf);
        }

        sem_lock(sem);

        status->running = 0;

        sem_unlock(sem);

        printf("[network] STOP command received, system will exit\n");
        log_warn("[network] STOP command received, system will exit");
        break;

    case CMD_UNKNOWN:
    default:
        protocol_build_error("unknown command",
                             reply_buf,
                             sizeof(reply_buf));
        tcp_client_send(sockfd, reply_buf);

        printf("[network] unknown command: %s\n", cmd.raw);
        log_warn("[network] unknown command: %s", cmd.raw);
        break;
    }
}

int main(void)
{
    system_status_t *status = NULL;
    sem_t *sem = NULL;

    monitor_config_t cfg;

    int sockfd = -1;
    int upload_interval = 1;

    char send_buf[SEND_BUF_SIZE];
    char cmd_buf[CMD_BUF_SIZE];

    /*
     * 打开已有共享内存
     */
    status = shm_open_existing();
    if (status == NULL)
    {
        printf("[network] shm_open_existing failed\n");
        return 1;
    }

    /*
     * 打开已有信号量
     */
    sem = sem_open_existing();
    if (sem == NULL)
    {
        shm_detach(status);
        return 1;
    }

    /*
     * 初始化日志模块
     */
    if (log_init() != 0)
    {
        printf("[network] log_init failed\n");
    }
    else
    {
        log_info("[network] start");
    }

    /*
     * 加载配置文件
     */
    if (config_load(CONFIG_FILE_PATH, &cfg) != 0)
    {
        printf("[network] config_load failed, use default config\n");
        config_set_default(&cfg);
        log_warn("[network] config_load failed, use default config");
    }
    else
    {
        log_info("[network] config_load success");
    }

    /*
     * 使用配置文件中的上传周期
     */
    upload_interval = cfg.sample_interval;

    /*
     * 连接 TCP 服务端
     */
    sockfd = tcp_client_connect(cfg.server_ip, cfg.server_port);
    if (sockfd == -1)
    {
        printf("[network] tcp connect failed, ip=%s port=%d\n",
               cfg.server_ip,
               cfg.server_port);

        log_error("[network] tcp connect failed, ip=%s port=%d",
                  cfg.server_ip,
                  cfg.server_port);
    }
    else
    {
        printf("[network] tcp connect success, ip=%s port=%d\n",
               cfg.server_ip,
               cfg.server_port);

        log_info("[network] tcp connect success, ip=%s port=%d",
                 cfg.server_ip,
                 cfg.server_port);
    }

    /*
     * 主循环
     */
    while (1)
    {
        system_status_t current_status;
        int recv_len;

        /*
         * 读取共享内存中的当前状态
         */
        sem_lock(sem);

        if (status->running == 0)
        {
            sem_unlock(sem);
            break;
        }

        current_status = *status;

        sem_unlock(sem);

        /*
         * 如果 socket 未连接，则尝试重连
         */
        if (sockfd == -1)
        {
            sockfd = tcp_client_connect(cfg.server_ip, cfg.server_port);

            if (sockfd == -1)
            {
                printf("[network] reconnect failed\n");
                log_warn("[network] reconnect failed");

                sleep(2);
                continue;
            }

            printf("[network] reconnect success\n");
            log_info("[network] reconnect success");
        }

        /*
         * 组装 JSON 格式上报数据
         */
        memset(send_buf, 0, sizeof(send_buf));

        /*
        * 构造状态上报 JSON
        */
        if (protocol_build_status_json(&current_status,
                                    send_buf,
                                    sizeof(send_buf)) != 0)
        {
            printf("[network] build status json failed\n");
            log_warn("[network] build status json failed");

            sleep(1);
            continue;
        }

        /*
         * 发送状态数据到服务器
         */
        if (tcp_client_send(sockfd, send_buf) != 0)
        {
            printf("[network] send failed, close socket\n");
            log_warn("[network] send failed, close socket");

            tcp_client_close(sockfd);
            sockfd = -1;

            sleep(1);
            continue;
        }

        printf("[network] send: %s", send_buf);

        /*
         * 尝试接收服务器命令
         *
         * timeout = 1 秒
         * 如果没有命令，会返回 0，不影响主循环。
         */
        memset(cmd_buf, 0, sizeof(cmd_buf));

        recv_len = tcp_client_recv_timeout(sockfd,
                                           cmd_buf,
                                           sizeof(cmd_buf),
                                           1);

        if (recv_len > 0)
        {


            handle_server_command(cmd_buf,
                                  status,
                                  sem,
                                  &upload_interval,
                                   sockfd);
        }
        else if (recv_len < 0)
        {
            /*
             * 连接异常，关闭 socket，后续自动重连
             */
            printf("[network] recv failed, close socket\n");
            log_warn("[network] recv failed, close socket");

            tcp_client_close(sockfd);
            sockfd = -1;

            sleep(1);
            continue;
        }

        /*
         * 根据上传周期休眠
         */
        sleep(upload_interval);
    }

    /*
     * 退出前关闭 socket
     */
    if (sockfd != -1)
    {
        tcp_client_close(sockfd);
        sockfd = -1;
    }

    log_info("[network] exit");

    sem_close_handle(sem);
    shm_detach(status);

    printf("[network] exit\n");

    return 0;
}