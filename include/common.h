#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#define SHM_NAME "/dm_shm"
#define SEM_NAME "/dm_sem"

typedef struct {
    float cpu_usage;
    float mem_usage;
    float temp_acpitz_1;
    float temp_acpitz_2;
    float temp_cpu;
    int running;
    float disk_usage;  /* 磁盘使用率 */
    float rx_kbps;      /* 网络接收速率 KB/s */
    float tx_kbps;      /* 网络发送速率 KB/s */
} system_status_t;

#endif // COMMON_H