#ifndef CPU_H
#define CPU_H

/*
 * 功能：
 *   获取 CPU 使用率
 *
 * 说明：
 *   旧版本：函数内部会 sleep(1)
 */
float get_cpu_usage(void);

/*
 * 功能：
 *   获取 CPU 使用率，非阻塞版本
 *
 * 参数：
 *   usage：输出 CPU 使用率
 *
 * 返回值：
 *   0：成功
 *   1：第一次调用，只完成初始化，usage 返回 0
 *  -1：失败
 *
 * 说明：
 *   该函数内部不会 sleep。
 *   它会用本次采样值和上一次采样值做差，计算 CPU 使用率。
 */
int get_cpu_usage_fast(float *usage);

#endif