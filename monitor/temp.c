#include <stdio.h>
#include "temp.h"

/*
 * 功能：读取系统温度
 * 默认路径：
 *   /sys/class/thermal/thermal_zone0/temp
 *
 * 说明：
 *   很多 Linux 系统该文件中的值单位是“毫摄氏度”
 *   例如：
 *     42000 -> 42.0℃
 */
float get_temperature(int zone)
{
    FILE *fp = NULL;
    int temp_milli = 0;  /* 毫摄氏度 */
    float temp_c = -1.0f;
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/thermal/thermal_zone%d/temp", zone);
    /* 打开温度文件 */
    fp = fopen(path, "r");
    if (fp == NULL)
    {
        perror("fopen temperature file failed");
        return -1.0f;
    }

    /* 读取整数温度值 */
    if (fscanf(fp, "%d", &temp_milli) != 1)
    {
        fclose(fp);
        return -1.0f;
    }

    fclose(fp);

    /* 转换为摄氏度 */
    temp_c = temp_milli / 1000.0f;

    return temp_c;
}