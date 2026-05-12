#include <stdio.h>

#include "disk.h"

int main(void)
{
    float disk_usage;

    /*
     * 检测根文件系统 "/" 的磁盘使用率
     */
    disk_usage = get_disk_usage("/");

    if (disk_usage < 0.0f)
    {
        printf("get_disk_usage failed\n");
        return 1;
    }

    printf("disk usage = %.2f%%\n", disk_usage);

    return 0;
}