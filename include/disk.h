#ifndef DISK_H
#define DISK_H

/*
 * 功能：
 *   获取指定路径所在文件系统的磁盘使用率
 *
 * 参数：
 *   path：路径，例如 "/" 表示根文件系统
 *
 * 返回值：
 *   成功：返回磁盘使用率，例如 35.6 表示 35.6%
 *   失败：返回 -1.0f
 */
float get_disk_usage(const char *path);

#endif