#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "shm.h"
#include "sem.h"
#include "msgq.h"
#include "log.h"
#include "config.h"

/*
 * 功能：
 *   启动 collector 子进程
 *
 * 返回值：
 *   成功：返回 collector 子进程 PID
 *   失败：返回 -1
 *
 * 说明：
 *   fork() 创建子进程
 *   子进程中使用 execl() 执行 ./collector
 */
static pid_t start_collector(void)
{
    pid_t pid;

    pid = fork();

    if (pid < 0)
    {
        /*
         * fork 失败
         */
        perror("fork collector failed");
        return -1;
    }
    else if (pid == 0)
    {
        /*
         * 子进程执行 collector 程序
         */
        execl("./collector", "./collector", NULL);

        /*
         * execl 成功不会返回
         * 只有失败才会执行到这里
         */
        perror("execl collector failed");
        exit(1);
    }

    /*
     * 父进程返回子进程 PID
     */
    return pid;
}

/*
 * 功能：
 *   启动 network 子进程
 *
 * 返回值：
 *   成功：返回 network 子进程 PID
 *   失败：返回 -1
 */
static pid_t start_network(void)
{
    pid_t pid;

    pid = fork();

    if (pid < 0)
    {
        perror("fork network failed");
        return -1;
    }
    else if (pid == 0)
    {
        /*
         * 子进程执行 network 程序
         */
        execl("./network", "./network", NULL);

        perror("execl network failed");
        exit(1);
    }

    return pid;
}

/*
 * 功能：
 *   启动 logger 子进程
 *
 * 返回值：
 *   成功：返回 logger 子进程 PID
 *   失败：返回 -1
 */
static pid_t start_logger(void)
{
    pid_t pid;

    pid = fork();

    if (pid < 0)
    {
        perror("fork logger failed");
        return -1;
    }
    else if (pid == 0)
    {
        /*
         * 子进程执行 logger 程序
         */
        execl("./logger", "./logger", NULL);

        perror("execl logger failed");
        exit(1);
    }

    return pid;
}

int main(void)
{
    pid_t pid_collector;
    pid_t pid_network;
    pid_t pid_logger;

    system_status_t *status = NULL;
    sem_t *sem = NULL;
    int msg_id = -1;
    monitor_config_t cfg;
    /* 创建共享内存 */
    status = shm_create();
    if (status == NULL)
    {
        return 1;
    }

    /* 创建信号量 */
    sem = sem_create();
    if (sem == NULL)
    {
        shm_detach(status);
        shm_remove();
        return 1;
    }

    msg_id = msgq_create();
    if (msg_id == -1)
    {
        sem_close_handle(sem);
        sem_remove();
        shm_detach(status);
        shm_remove();
        return 1;
    }

    if (log_init() != 0)
    {
        printf("[manager] log init failed\n");
    }
    
    /*
    * 启动 logger
    */
    pid_logger = start_logger();
    if (pid_logger < 0)
    {
        msgq_remove(msg_id);
        sem_close_handle(sem);
        sem_remove();
        shm_detach(status);
        shm_remove();
        return 1;
    }

    /*
    * manager 自己写日志
    */
    log_info("[manager] start");

    /*
    * 启动 collector
    */
    pid_collector = start_collector();
    if (pid_collector < 0)
    {
        log_error("[manager] start collector failed");

        log_send_exit();
        waitpid(pid_logger, NULL, 0);

        msgq_remove(msg_id);
        sem_close_handle(sem);
        sem_remove();
        shm_detach(status);
        shm_remove();
        return 1;
    }

    /*
    * 启动 network
    */
    pid_network = start_network();
    if (pid_network < 0)
    {
        log_error("[manager] start network failed");

        /*
        * 通知 collector 退出
        */
        sem_lock(sem);
        status->running = 0;
        sem_unlock(sem);

        waitpid(pid_collector, NULL, 0);

        log_send_exit();
        waitpid(pid_logger, NULL, 0);

        msgq_remove(msg_id);
        sem_close_handle(sem);
        sem_remove();
        shm_detach(status);
        shm_remove();
        return 1;
    }

    /* 加载配置文件 */
    if (config_load(CONFIG_FILE_PATH, &cfg) != 0)
    {
        printf("[manager] config_load failed, use default config\n");
        config_set_default(&cfg);
    }

    log_info("manager start");

    printf("[manager] run_time = %d seconds\n", cfg.run_time);

/*
 * manager 主循环
 *
 * 功能：
 *   1. 检查系统 running 标志
 *   2. 检查 collector 是否异常退出
 *   3. 检查 network 是否异常退出
 *   4. 如果系统仍在运行，则自动重启异常退出的子进程
 */
while (1)
{
    int running = 1;
    int child_status;
    pid_t ret;

    /*
     * 读取系统运行标志
     */
    sem_lock(sem);

    running = status->running;

    sem_unlock(sem);

    /*
     * 如果 running 为 0，说明系统需要退出
     * 例如 network 收到了 STOP 命令
     */
    if (running == 0)
    {
        break;
    }

    /*
     * 检查 collector 是否退出
     *
     * WNOHANG：
     *   不阻塞等待
     *   如果子进程没退出，waitpid 立即返回 0
     */
    ret = waitpid(pid_collector, &child_status, WNOHANG);

    if (ret == pid_collector)
    {
        /*
         * collector 已经退出
         */
        if (WIFEXITED(child_status))
        {
            log_warn("[manager] collector exited, code=%d",
                     WEXITSTATUS(child_status));
        }
        else if (WIFSIGNALED(child_status))
        {
            log_warn("[manager] collector killed by signal=%d",
                     WTERMSIG(child_status));
        }
        else
        {
            log_warn("[manager] collector exited unexpectedly");
        }

        /*
         * 如果系统仍在 running，则自动重启 collector
         */
        pid_collector = start_collector();

        if (pid_collector < 0)
        {
            log_error("[manager] restart collector failed");

            /*
             * 重启失败，系统进入停止状态
             */
            sem_lock(sem);
            status->running = 0;
            sem_unlock(sem);

            break;
        }

        log_info("[manager] collector restarted, pid=%d", pid_collector);
    }
    else if (ret < 0)
    {
        /*
         * waitpid 出错，一般说明 pid 不合法或子进程状态异常
         */
        perror("waitpid collector failed");
        log_warn("[manager] waitpid collector failed");
    }

    /*
     * 检查 network 是否退出
     */
    ret = waitpid(pid_network, &child_status, WNOHANG);

    if (ret == pid_network)
    {
        /*
         * network 已经退出
         */
        if (WIFEXITED(child_status))
        {
            log_warn("[manager] network exited, code=%d",
                     WEXITSTATUS(child_status));
        }
        else if (WIFSIGNALED(child_status))
        {
            log_warn("[manager] network killed by signal=%d",
                     WTERMSIG(child_status));
        }
        else
        {
            log_warn("[manager] network exited unexpectedly");
        }

        /*
         * 如果系统仍在 running，则自动重启 network
         */
        pid_network = start_network();

        if (pid_network < 0)
        {
            log_error("[manager] restart network failed");

            sem_lock(sem);
            status->running = 0;
            sem_unlock(sem);

            break;
        }

        log_info("[manager] network restarted, pid=%d", pid_network);
    }
    else if (ret < 0)
    {
        perror("waitpid network failed");
        log_warn("[manager] waitpid network failed");
    }

    /*
     * 每 1 秒检查一次子进程状态
     */
    sleep(1);
    }
    /*
    * 通知 collector 和 network 退出
    */
    sem_lock(sem);
    status->running = 0;
    sem_unlock(sem);
    /* 等待三个子进程结束 */
    /*
    * 回收 collector
    *
    * 如果子进程已经被前面的 WNOHANG 回收过，
    * waitpid 可能返回 -1，这里不作为严重错误处理。
    */
    if (pid_collector > 0)
    {
        waitpid(pid_collector, NULL, 0);
    }

    /*
    * 回收 network
    */
    if (pid_network > 0)
    {
        waitpid(pid_network, NULL, 0);
    }

    log_info("[manager] collector exited");
    log_info("[manager] network exited");
    log_send_exit();
    
    log_info("logger exited");
    waitpid(pid_logger, NULL, 0);

    /* 清理资源 */
    sem_close_handle(sem);
    sem_remove();

    shm_detach(status);
    shm_remove();  

    msgq_remove(msg_id);

    printf("[manager] all child process exited, cleanup done\n");

    return 0;
}