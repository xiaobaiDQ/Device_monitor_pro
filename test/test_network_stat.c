#include <stdio.h>

#include "network_stat.h"

/*
 * 修改成你自己设备上的真实网卡名
 *
 * 查看方式：
 *   cat /proc/net/dev
 *
 * 常见网卡名：
 *   eth0
 *   wlan0
 *   enp3s0
 *   ens33
 */
#define TEST_IFACE "enp1s0"

int main(void)
{
    network_rate_t rate;

    /*
     * 获取指定网卡的网络速率
     *
     * 注意：
     *   get_network_rate 内部会等待 1 秒
     */
    if (get_network_rate(TEST_IFACE, &rate) != 0)
    {
        printf("get_network_rate failed, iface=%s\n", TEST_IFACE);
        return 1;
    }

    printf("iface = %s\n", TEST_IFACE);
    printf("rx_rate = %.2f KB/s\n", rate.rx_kbps);
    printf("tx_rate = %.2f KB/s\n", rate.tx_kbps);

    return 0;
}