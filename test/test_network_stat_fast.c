#include <stdio.h>
#include <unistd.h>

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
    int ret;

    /*
     * 第一次调用：只初始化
     */
    ret = get_network_rate_fast(TEST_IFACE, 1, &rate);
    if (ret < 0)
    {
        printf("get_network_rate_fast init failed, iface=%s\n", TEST_IFACE);
        return 1;
    }

    printf("first call, rx=%.2f KB/s, tx=%.2f KB/s, ret=%d\n",
           rate.rx_kbps,
           rate.tx_kbps,
           ret);

    /*
     * 模拟采样周期：等待 1 秒
     */
    sleep(1);

    /*
     * 第二次调用：可以计算速率
     */
    ret = get_network_rate_fast(TEST_IFACE, 1, &rate);
    if (ret < 0)
    {
        printf("get_network_rate_fast failed, iface=%s\n", TEST_IFACE);
        return 1;
    }

    printf("second call, rx=%.2f KB/s, tx=%.2f KB/s, ret=%d\n",
           rate.rx_kbps,
           rate.tx_kbps,
           ret);

    /*
     * 连续测试几次
     */
    for (int i = 0; i < 5; i++)
    {
        sleep(1);

        ret = get_network_rate_fast(TEST_IFACE, 1, &rate);
        if (ret == 0)
        {
            printf("rx=%.2f KB/s, tx=%.2f KB/s\n",
                   rate.rx_kbps,
                   rate.tx_kbps);
        }
        else if (ret == 1)
        {
            printf("network init only\n");
        }
        else
        {
            printf("network read failed\n");
        }
    }

    return 0;
}