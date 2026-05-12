#include <stdio.h>
#include <string.h>
#include "memory.h"

/*
 * 功能：读取 /proc/meminfo，计算内存使用率
 * 公式：
 *   usage = (MemTotal - MemAvailable) / MemTotal * 100
 *
 * 说明：
 *   MemTotal     : 总内存
 *   MemAvailable : 当前可用内存（更适合直接拿来算）
 */
float get_memory_usage(void)
{
    FILE *fp = NULL;
    char line[256];

    long mem_total = 0;      /* 总内存，单位 kB */
    long mem_available = 0;  /* 可用内存，单位 kB */

    float usage = -1.0f;

    /* 打开 /proc/meminfo 文件 */
    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL)
    {
        perror("fopen /proc/meminfo failed");
        return -1.0f;
    }

    /* 按行读取文件内容 */
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        /*
         * 解析 MemTotal
         * 示例：
         *   MemTotal:       16329684 kB
         */
        if (strncmp(line, "MemTotal:", 9) == 0)
        {
            sscanf(line, "MemTotal: %ld kB", &mem_total);
        }
        /*
         * 解析 MemAvailable
         * 示例：
         *   MemAvailable:    8245632 kB
         */
        else if (strncmp(line, "MemAvailable:", 13) == 0)
        {
            sscanf(line, "MemAvailable: %ld kB", &mem_available);
        }

        /* 如果两个关键值都拿到了，就可以提前结束 */
        if (mem_total > 0 && mem_available > 0)
        {
            break;
        }
    }

    fclose(fp);

    /* 防止除零或读取失败 */
    if (mem_total <= 0)
    {
        return -1.0f;
    }

    /* 计算内存使用率 */
    usage = (float)(mem_total - mem_available) / (float)mem_total * 100.0f;

    return usage;
}