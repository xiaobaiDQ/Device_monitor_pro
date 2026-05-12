#ifndef TEMP_H
#define TEMP_H

/*
 * 功能：读取系统温度
 * 返回值：
 *   成功：返回温度值（单位：摄氏度）
 *   失败：返回 -1.0f
 */
float get_temperature(int zone);

#endif