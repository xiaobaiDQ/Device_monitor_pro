#include <stdio.h>
#include <unistd.h>

#include "cpu.h"

int main(void)
{
    float cpu_usage = 0.0f;
    int ret;

    /*
     * 第一次调用：只初始化
     */
    ret = get_cpu_usage_fast(&cpu_usage);
    if (ret < 0)
    {
        printf("get_cpu_usage_fast init failed\n");
        return 1;
    }

    printf("first call, cpu_usage = %.2f%%, ret = %d\n", cpu_usage, ret);

    /*
     * 等待 1 秒，模拟 collector 的采样周期
     */
    sleep(1);

    /*
     * 第二次调用：可以计算真实 CPU 使用率
     */
    ret = get_cpu_usage_fast(&cpu_usage);
    if (ret < 0)
    {
        printf("get_cpu_usage_fast failed\n");
        return 1;
    }

    printf("second call, cpu_usage = %.2f%%, ret = %d\n", cpu_usage, ret);

    /*
     * 连续测试几次
     */
    for (int i = 0; i < 5; i++)
    {
        sleep(1);

        ret = get_cpu_usage_fast(&cpu_usage);
        if (ret == 0)
        {
            printf("cpu_usage = %.2f%%\n", cpu_usage);
        }
        else if (ret == 1)
        {
            printf("cpu init only\n");
        }
        else
        {
            printf("cpu read failed\n");
        }
    }

    return 0;
}