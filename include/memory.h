#ifndef MEMORY_H
#define MEMORY_H

/* 
 * 功能：读取系统内存使用率
 * 返回值：
 *   成功：返回内存使用率（百分比）
 *   失败：返回 -1.0f
 */
float get_memory_usage(void);

#endif