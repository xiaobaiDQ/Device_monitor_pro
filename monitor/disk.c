#include <stdio.h>
#include <sys/statvfs.h>

#include "disk.h"

/*
 * 函数名：
 *   get_disk_usage
 *
 * 功能：
 *   获取指定路径所在文件系统的磁盘使用率
 *
 * 参数：
 *   path：要检测的路径
 *         常用 "/" 表示根文件系统
 *
 * 使用函数：
 *   statvfs()
 *
 * statvfs 结构体中常用字段：
 *   f_blocks：文件系统总块数
 *   f_bavail：普通用户可用块数
 *   f_bsize ：每个块的大小
 *
 * 计算思路：
 *   total = f_blocks
 *   free  = f_bavail
 *   used  = total - free
 *   usage = used / total * 100
 *
 * 返回值：
 *   成功：返回磁盘使用率
 *   失败：返回 -1.0f
 */
float get_disk_usage(const char *path)
{
    struct statvfs vfs;

    unsigned long long total_blocks;
    unsigned long long free_blocks;
    unsigned long long avail_blocks;
    unsigned long long used_blocks;

    float usage;

    /*
     * 参数检查
     */
    if (path == NULL)
    {
        return -1.0f;
    }

    /*
     * 获取指定路径所在文件系统的信息
     */
    if (statvfs(path, &vfs) != 0)
    {
        perror("statvfs failed");
        return -1.0f;
    }

    /*
     * 防止除以 0
     */
    if (vfs.f_blocks == 0)
    {
        return -1.0f;
    }

    /*
     * 取出总块数、可用块数（含 root 保留）和
     * 非 root 可用块数（不含 root 保留）
     */
    total_blocks = vfs.f_blocks;
    free_blocks  = vfs.f_bfree;
    avail_blocks = vfs.f_bavail;

    /*
     * 计算已用块数
     */
    used_blocks = total_blocks - free_blocks;

    /*
     * 计算磁盘使用率
     *
     * 与 df 保持一致：
     *   分母 = used + avail（排除 root 保留块）
     *   分子 = used
     */
    if (used_blocks + avail_blocks == 0)
    {
        return -1.0f;
    }

    usage = (float)used_blocks / (float)(used_blocks + avail_blocks) * 100.0f;

    return usage;
}