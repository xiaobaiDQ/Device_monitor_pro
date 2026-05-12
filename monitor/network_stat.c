#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "network_stat.h"

/*
 * 网络字节统计结构体
 *
 * rx_bytes:
 *   网卡累计接收字节数
 *
 * tx_bytes:
 *   网卡累计发送字节数
 */
typedef struct
{
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
} network_bytes_t;

/*
 * 函数名：
 *   read_network_bytes
 *
 * 功能：
 *   从 /proc/net/dev 中读取指定网卡的累计接收/发送字节数
 *
 * 参数：
 *   iface：网卡名，例如 "eth0"、"wlan0"、"enp3s0"
 *   bytes：输出读取到的 rx_bytes 和 tx_bytes
 *
 * 返回值：
 *   成功：0
 *   失败：-1
 *
 * /proc/net/dev 数据格式示例：
 *
 *   face |bytes packets errs drop fifo frame compressed multicast|bytes packets errs drop fifo colls carrier compressed
 *   eth0: 12345  100     0    0    0    0     0          0        67890 200 ...
 *
 * 注意：
 *   冒号左边是网卡名
 *   冒号右边第 1 个字段是 rx_bytes
 *   冒号右边第 9 个字段是 tx_bytes
 */
static int read_network_bytes(const char *iface, network_bytes_t *bytes)
{
    FILE *fp = NULL;
    char line[512];

    if (iface == NULL || bytes == NULL)
    {
        return -1;
    }

    fp = fopen("/proc/net/dev", "r");
    if (fp == NULL)
    {
        perror("fopen /proc/net/dev failed");
        return -1;
    }

    /*
     * 一行一行读取 /proc/net/dev
     */
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        char ifname[64];
        unsigned long long rx_bytes = 0;
        unsigned long long tx_bytes = 0;

        /*
         * 这里用 sscanf 解析一整行
         *
         * 格式说明：
         *   %63[^:]:
         *     读取冒号前的网卡名
         *
         *   后面的字段中：
         *     第 1 个是 rx_bytes
         *     第 9 个是 tx_bytes
         *
         * 注意：
         *   %*llu 表示读取但丢弃该字段
         */
        int ret = sscanf(line,
                         " %63[^:]: %llu %*llu %*llu %*llu %*llu %*llu %*llu %*llu %llu",
                         ifname,
                         &rx_bytes,
                         &tx_bytes);

        /*
         * ret == 3 表示成功读到了：
         *   ifname
         *   rx_bytes
         *   tx_bytes
         */
        if (ret == 3)
        {
            /*
             * 判断是不是目标网卡
             */
            if (strcmp(ifname, iface) == 0)
            {
                bytes->rx_bytes = rx_bytes;
                bytes->tx_bytes = tx_bytes;

                fclose(fp);
                return 0;
            }
        }
    }

    fclose(fp);

    /*
     * 没找到指定网卡
     */
    return -1;
}

/*
 * 函数名：
 *   get_network_rate
 *
 * 功能：
 *   获取指定网卡 1 秒内的 RX/TX 速率
 *
 * 计算公式：
 *   rx_kbps = (rx2 - rx1) / 1024.0
 *   tx_kbps = (tx2 - tx1) / 1024.0
 *
 * 因为两次采样间隔是 1 秒，所以单位就是 KB/s。
 */
int get_network_rate(const char *iface, network_rate_t *rate)
{
    network_bytes_t b1;
    network_bytes_t b2;

    unsigned long long rx_diff;
    unsigned long long tx_diff;

    if (iface == NULL || rate == NULL)
    {
        return -1;
    }

    /*
     * 第一次采样
     */
    if (read_network_bytes(iface, &b1) != 0)
    {
        return -1;
    }

    /*
     * 等待 1 秒
     */
    sleep(1);

    /*
     * 第二次采样
     */
    if (read_network_bytes(iface, &b2) != 0)
    {
        return -1;
    }

    /*
     * 防止极端情况下计数器回绕
     * 正常情况下 b2 应该大于等于 b1
     */
    if (b2.rx_bytes < b1.rx_bytes || b2.tx_bytes < b1.tx_bytes)
    {
        return -1;
    }

    rx_diff = b2.rx_bytes - b1.rx_bytes;
    tx_diff = b2.tx_bytes - b1.tx_bytes;

    /*
     * 字节数转换成 KB/s
     */
    rate->rx_kbps = (float)rx_diff / 1024.0f;
    rate->tx_kbps = (float)tx_diff / 1024.0f;

    return 0;
}

/*
 * 函数名：
 *   get_network_rate_fast
 *
 * 功能：
 *   获取指定网卡的网络收发速率，非阻塞版本
 *
 * 工作方式：
 *   第一次调用：
 *     读取当前 rx_bytes / tx_bytes
 *     保存为 last_bytes
 *     不计算速率
 *
 *   第二次及以后调用：
 *     再次读取当前 rx_bytes / tx_bytes
 *     用当前值减去上一次值
 *     再除以 interval_sec
 *     得到 KB/s
 *
 * 返回值：
 *   0：成功计算出网络速率
 *   1：第一次调用，仅初始化
 *  -1：失败
 */
int get_network_rate_fast(const char *iface,
                          int interval_sec,
                          network_rate_t *rate)
{
    static int initialized = 0;
    static network_bytes_t last_bytes;

    network_bytes_t current_bytes;

    unsigned long long rx_diff;
    unsigned long long tx_diff;

    if (iface == NULL || rate == NULL || interval_sec <= 0)
    {
        return -1;
    }

    /*
     * 先读取当前网卡累计收发字节数
     */
    if (read_network_bytes(iface, &current_bytes) != 0)
    {
        return -1;
    }

    /*
     * 第一次调用时没有“上一次数据”
     * 所以只保存当前值，不计算速率
     */
    if (!initialized)
    {
        last_bytes = current_bytes;
        initialized = 1;

        rate->rx_kbps = 0.0f;
        rate->tx_kbps = 0.0f;

        return 1;
    }

    /*
     * 防止极端情况下计数器回绕
     */
    if (current_bytes.rx_bytes < last_bytes.rx_bytes ||
        current_bytes.tx_bytes < last_bytes.tx_bytes)
    {
        last_bytes = current_bytes;
        return -1;
    }

    /*
     * 计算两次采样之间的字节差
     */
    rx_diff = current_bytes.rx_bytes - last_bytes.rx_bytes;
    tx_diff = current_bytes.tx_bytes - last_bytes.tx_bytes;

    /*
     * 更新上一次采样值
     */
    last_bytes = current_bytes;

    /*
     * 字节差 / 时间间隔 / 1024 = KB/s
     */
    rate->rx_kbps = (float)rx_diff / (float)interval_sec / 1024.0f;
    rate->tx_kbps = (float)tx_diff / (float)interval_sec / 1024.0f;

    return 0;
}