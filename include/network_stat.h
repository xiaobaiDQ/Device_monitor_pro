#ifndef NETWORK_STAT_H
#define NETWORK_STAT_H

/*
 * 网络速率结构体
 *
 * rx_kbps:
 *   接收速率，单位 KB/s
 *
 * tx_kbps:
 *   发送速率，单位 KB/s
 */
typedef struct
{
    float rx_kbps;
    float tx_kbps;
} network_rate_t;

/*
 * 功能：
 *   获取指定网卡的网络收发速率
 *
 * 参数：
 *   iface：网卡名称，例如 "eth0"、"wlan0"、"enp3s0"
 *   rate ：用于保存计算出来的网络速率
 *
 * 返回值：
 *   成功：0
 *   失败：-1
 *
 * 注意：
 *   旧版本：该函数内部会 sleep(1)，因为网络速率需要两次采样做差值。
 */
int get_network_rate(const char *iface, network_rate_t *rate);

/*
 * 功能：
 *   获取指定网卡的网络收发速率，非阻塞版本
 *
 * 参数：
 *   iface        ：网卡名称，例如 "eth0"、"wlan0"、"enp3s0"
 *   interval_sec ：两次采样之间的时间间隔，单位秒
 *   rate         ：用于保存计算出来的网络速率
 *
 * 返回值：
 *   0：成功计算出网络速率
 *   1：第一次调用，只完成初始化，rate 返回 0
 *  -1：失败
 *
 * 说明：
 *   该函数内部不会 sleep。
 *   它会使用本次采样值和上一次采样值做差。
 */
int get_network_rate_fast(const char *iface,
                          int interval_sec,
                          network_rate_t *rate);

#endif