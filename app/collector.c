#include <stdio.h>
#include <unistd.h>
#include "config.h"
#include "shm.h"
#include "sem.h"
#include "memory.h"
#include "temp.h"
#include "cpu.h"
#include "log.h"
#include "disk.h"
#include "network_stat.h"

int main(void)
{
    system_status_t *status = NULL;
    sem_t *sem = NULL;

    float cpu = 10.0f;
    float mem = 20.0f;
    float temp_acpitz_1= 30.0f;
    float temp_acpitz_2= 30.0f;
    float temp_cpu= 30.0f;
    float disk = 0.0f;   /* 磁盘使用率 */
    network_rate_t net_rate;
    monitor_config_t cfg;
    config_load(CONFIG_FILE_PATH, &cfg);


    /* 打开已有共享内存 */
    status = shm_open_existing();
    if (status == NULL)
    {
        printf("[collector] shm_open_existing failed\n");
        return 1;
    }

    /* 打开已有信号量 */
    sem = sem_open_existing();
    if (sem == NULL)
    {
        shm_detach(status);
        return 1;
    }

    if(log_init() != 0)
    {
        printf("[collector] log_init failed\n");

    }
    else
    {
        log_info("[collector] log_init success");
    }
    
    /* 持续写入假数据 */
    while (1)
    {
        /* 先判断是否需要退出 */
        sem_lock(sem);
        if (status->running == 0)
        {
            sem_unlock(sem);
            break;
        }
        sem_unlock(sem);

        /* 模拟采集数据 */
         cpu += 1.0f;
         if(cpu > 100.0f) {
             cpu = 0.0f;
         }
        // mem += 1.0f;
        // temp += 1.0f;


        mem = get_memory_usage();
        if(mem < 0) {
            mem = 0.0f;
            log_error("[collector] get_memory_usage failed");
        }
        temp_acpitz_1 = get_temperature(0);
        if(temp_acpitz_1 < 0) {
            temp_acpitz_1 = 0.0f;
            log_warn("[collector] get_temperature(0) failed");
        }
        temp_acpitz_2 = get_temperature(1);
        if(temp_acpitz_2 < 0) {
            temp_acpitz_2 = 0.0f;
            log_warn("[collector] get_temperature(1) failed");
        }
        temp_cpu = get_temperature(2);
        if(temp_cpu < 0) {
            temp_cpu = 0.0f;
            log_warn("[collector] get_temperature(2) failed");
        }
        if (get_cpu_usage_fast(&cpu) < 0)
        {
            printf("[collector] get_cpu_usage_fast failed\n");
            log_warn("[collector] get_cpu_usage_fast failed");
            cpu = 0.0f;
        }
        if(cpu < 0) {
            cpu = 0.0f;
            log_error("[collector] get_cpu_usage failed");
        }
        /*
        * 真实读取磁盘使用率
        * 这里检测根文件系统 "/"
        */
        disk = get_disk_usage("/");

        if (disk < 0.0f)
        {
            printf("[collector] get_disk_usage failed\n");
            log_error("[collector] get_disk_usage failed");
            disk = 0.0f;
        }

       /*
        * 非阻塞方式读取网络收发速率
        *
        * 返回 1 表示第一次调用，只完成初始化；
        * 这种情况不算错误。
        */
        int net_ret = get_network_rate_fast(cfg.network_iface,
                                            cfg.sample_interval,
                                            &net_rate);

        if (net_ret < 0)
        {
            printf("[collector] get_network_rate_fast failed, iface=%s\n",
                cfg.network_iface);

            log_warn("[collector] get_network_rate_fast failed, iface=%s",
                    cfg.network_iface);

            net_rate.rx_kbps = 0.0f;
            net_rate.tx_kbps = 0.0f;
        }
        sem_lock(sem);
        /* 写入共享内存 */
        status->cpu_usage = cpu;
        status->mem_usage = mem;
        status->temp_acpitz_1 = temp_acpitz_1;
        status->temp_acpitz_2 = temp_acpitz_2;
        status->temp_cpu = temp_cpu;
        status->disk_usage = disk;
        status->rx_kbps = net_rate.rx_kbps;
        status->tx_kbps = net_rate.tx_kbps;
        printf("[collector] write: cpu=%.1f mem=%.1f temp_acpitz_1=%.1f temp_acpitz_2=%.1f temp_cpu=%.1f\n disk=%.1f rx=%.1f tx=%.1f\n",
               status->cpu_usage,
               status->mem_usage,
               status->temp_acpitz_1,
               status->temp_acpitz_2,
               status->temp_cpu,
               status->disk_usage,
               status->rx_kbps,
               status->tx_kbps);

        sem_unlock(sem);

        sleep(cfg.sample_interval);
    }

    /* 清理资源 */
    sem_close_handle(sem);
    shm_detach(status);
    log_info("collector exit");
    printf("[collector] exit\n");
    return 0;
}