#include <stdio.h>
#include "cpu.h"

int main(void)
{
    float cpu_usage = 0.0f;

    /*
     * 调用 CPU 采集函数
     * 注意：函数内部会等待大约 1 秒
     */
    cpu_usage = get_cpu_usage();

    if (cpu_usage < 0.0f)
    {
        printf("get_cpu_usage failed\n");
        return 1;
    }

    printf("cpu usage = %.2f%%\n", cpu_usage);

    return 0;
}