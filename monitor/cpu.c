#include <stdio.h>
#include <unistd.h>
#include "cpu.h"

/*
 * cpu_time_t 用来保存一次 /proc/stat 中读取到的 CPU 时间数据
 *
 * /proc/stat 第一行大概长这样：
 * cpu  1234 56 789 101112 13 14 15 0 0 0
 *
 * 常见字段含义：
 * user    : 用户态时间
 * nice    : 低优先级用户态时间
 * system  : 内核态时间
 * idle    : 空闲时间
 * iowait  : 等待 IO 时间
 * irq     : 硬中断时间
 * softirq : 软中断时间
 * steal   : 虚拟化偷取时间
 */
typedef struct
{
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
} cpu_time_t;

/*
 * 功能：
 *   读取 /proc/stat 第一行 CPU 总时间
 *
 * 参数：
 *   t：用于保存读取到的 CPU 时间数据
 *
 * 返回值：
 *   成功：0
 *   失败：-1
 */
static int read_cpu_time(cpu_time_t *t)
{
    FILE *fp = NULL;
    char cpu_name[16];

    if (t == NULL)
    {
        return -1;
    }

    fp = fopen("/proc/stat", "r");
    if (fp == NULL)
    {
        perror("fopen /proc/stat failed");
        return -1;
    }

    /*
     * 只读取第一行，也就是所有 CPU 核心的总统计
     *
     * 第一列是 "cpu"
     * 后面依次读取各种时间字段
     */
    if (fscanf(fp,
               "%15s %llu %llu %llu %llu %llu %llu %llu %llu",
               cpu_name,
               &t->user,
               &t->nice,
               &t->system,
               &t->idle,
               &t->iowait,
               &t->irq,
               &t->softirq,
               &t->steal) != 9)
    {
        fclose(fp);
        return -1;
    }

    fclose(fp);

    return 0;
}

/*
 * 功能：
 *   计算一次采样中的总 CPU 时间
 *
 * 说明：
 *   /proc/stat 里的 CPU 时间单位通常是 jiffies
 *   我们不需要关心 jiffies 具体是多少毫秒，因为使用率只看两次采样差值比例
 */
static unsigned long long get_total_time(const cpu_time_t *t)
{
    return t->user +
           t->nice +
           t->system +
           t->idle +
           t->iowait +
           t->irq +
           t->softirq +
           t->steal;
}

/*
 * 功能：
 *   计算一次采样中的空闲时间
 *
 * 说明：
 *   idle + iowait 通常都算作空闲相关时间
 */
static unsigned long long get_idle_time(const cpu_time_t *t)
{
    return t->idle + t->iowait;
}

/*
 * 功能：
 *   获取 CPU 使用率
 *
 * 原理：
 *   第一次读取 CPU 时间
 *   等待 1 秒
 *   第二次读取 CPU 时间
 *   使用两次 total 和 idle 的差值计算 CPU 使用率
 *
 * 公式：
 *   CPU使用率 = (total_diff - idle_diff) / total_diff * 100
 */
float get_cpu_usage(void)
{
    cpu_time_t t1;
    cpu_time_t t2;

    unsigned long long total1 = 0;
    unsigned long long total2 = 0;
    unsigned long long idle1 = 0;
    unsigned long long idle2 = 0;

    unsigned long long total_diff = 0;
    unsigned long long idle_diff = 0;

    float usage = -1.0f;

    /*
     * 第一次采样
     */
    if (read_cpu_time(&t1) != 0)
    {
        return -1.0f;
    }

    /*
     * 等待 1 秒后进行第二次采样
     * 注意：这会导致调用 get_cpu_usage() 的线程阻塞约 1 秒
     */
    sleep(1);

    /*
     * 第二次采样
     */
    if (read_cpu_time(&t2) != 0)
    {
        return -1.0f;
    }

    total1 = get_total_time(&t1);
    total2 = get_total_time(&t2);

    idle1 = get_idle_time(&t1);
    idle2 = get_idle_time(&t2);

    total_diff = total2 - total1;
    idle_diff = idle2 - idle1;

    /*
     * 防止除以 0
     */
    if (total_diff == 0)
    {
        return -1.0f;
    }

    usage = (float)(total_diff - idle_diff) / (float)total_diff * 100.0f;

    return usage;
}

/*
 * 功能：
 *   获取 CPU 使用率，非阻塞版本
 *
 * 工作方式：
 *   第一次调用：
 *     读取当前 CPU 时间，保存到 last_time，不计算使用率
 *
 *   第二次及以后调用：
 *     读取当前 CPU 时间
 *     和 last_time 做差
 *     计算 CPU 使用率
 *     更新 last_time
 *
 * 返回值：
 *   0：成功计算出 CPU 使用率
 *   1：第一次调用，仅初始化
 *  -1：失败
 */
int get_cpu_usage_fast(float *usage)
{
    static int initialized = 0;
    static cpu_time_t last_time;

    cpu_time_t current_time;

    unsigned long long total_last;
    unsigned long long total_current;
    unsigned long long idle_last;
    unsigned long long idle_current;

    unsigned long long total_diff;
    unsigned long long idle_diff;

    if (usage == NULL)
    {
        return -1;
    }

    /*
     * 读取当前 CPU 时间
     */
    if (read_cpu_time(&current_time) != 0)
    {
        return -1;
    }

    /*
     * 第一次调用只保存初始值
     * 因为没有“上一次数据”，所以暂时无法计算差值
     */
    if (!initialized)
    {
        last_time = current_time;
        initialized = 1;
        *usage = 0.0f;
        return 1;
    }

    total_last = get_total_time(&last_time);
    total_current = get_total_time(&current_time);

    idle_last = get_idle_time(&last_time);
    idle_current = get_idle_time(&current_time);

    /*
     * 防止异常情况
     */
    if (total_current < total_last || idle_current < idle_last)
    {
        last_time = current_time;
        return -1;
    }

    total_diff = total_current - total_last;
    idle_diff = idle_current - idle_last;

    /*
     * 更新上一次采样值
     */
    last_time = current_time;

    if (total_diff == 0)
    {
        return -1;
    }

    /*
     * CPU 使用率计算公式
     */
    *usage = (float)(total_diff - idle_diff) / (float)total_diff * 100.0f;

    return 0;
}