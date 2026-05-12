#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>

#include "tcp_client.h"

/*
 * 功能：
 *   连接 TCP 服务器
 */
int tcp_client_connect(const char *ip, int port)
{
    int sockfd;
    struct sockaddr_in server_addr;

    if (ip == NULL || port <= 0)
    {
        return -1;
    }

    /*
     * 创建 TCP socket
     *
     * AF_INET     ：IPv4
     * SOCK_STREAM ：TCP
     * 0           ：使用默认协议
     */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket failed");
        return -1;
    }

    /*
     * 清空服务器地址结构体
     */
    memset(&server_addr, 0, sizeof(server_addr));

    /*
     * 设置服务器地址信息
     */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    /*
     * 把字符串 IP 转换成网络字节序地址
     */
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton failed");
        close(sockfd);
        return -1;
    }

    /*
     * 连接服务器
     */
    if (connect(sockfd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) == -1)
    {
        perror("connect failed");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/*
 * 功能：
 *   发送字符串数据
 */
int tcp_client_send(int sockfd, const char *data)
{
    ssize_t ret;
    size_t len;

    if (sockfd < 0 || data == NULL)
    {
        return -1;
    }

    len = strlen(data);

    /*
     * send 发送数据
     *
     * 注意：
     *   这里发送的是字符串内容，不包含末尾 '\0'
     */
    ret = send(sockfd, data, len, 0);
    if (ret == -1)
    {
        perror("send failed");
        return -1;
    }

    /*
     * 简单判断是否完整发送
     * 当前数据很短，一般一次 send 就能发完
     * 后面如果发送大数据，可以封装循环发送
     */
    if ((size_t)ret != len)
    {
        printf("send incomplete\n");
        return -1;
    }

    return 0;
}

/*
 * 功能：
 *   带超时接收服务器下发的数据
 *
 * 作用：
 *   network 进程不能一直阻塞在 recv() 上，
 *   因为它还要周期性上传状态数据。
 *
 *   所以这里用 select() 等待一小段时间：
 *   - 有数据：recv 读取
 *   - 没数据：返回 0
 *   - 出错：返回 -1
 */
int tcp_client_recv_timeout(int sockfd, char *buf, int size, int timeout_sec)
{
    fd_set readfds;
    struct timeval tv;
    int ret;
    ssize_t n;

    if (sockfd < 0 || buf == NULL || size <= 0)
    {
        return -1;
    }

    /*
     * 清空 fd 集合
     */
    FD_ZERO(&readfds);

    /*
     * 把 sockfd 加入监听集合
     */
    FD_SET(sockfd, &readfds);

    /*
     * 设置超时时间
     */
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;

    /*
     * select 用来判断 sockfd 是否有数据可读
     */
    ret = select(sockfd + 1, &readfds, NULL, NULL, &tv);

    if (ret < 0)
    {
        perror("select failed");
        return -1;
    }

    if (ret == 0)
    {
        /*
         * 超时，没有收到数据
         */
        return 0;
    }

    /*
     * 有数据可读
     */
    if (FD_ISSET(sockfd, &readfds))
    {
        n = recv(sockfd, buf, size - 1, 0);

        if (n > 0)
        {
            /*
             * 手动补字符串结束符
             */
            buf[n] = '\0';
            return (int)n;
        }
        else if (n == 0)
        {
            /*
             * 对端关闭连接
             */
            printf("server closed connection\n");
            return -1;
        }
        else
        {
            perror("recv failed");
            return -1;
        }
    }

    return 0;
}

/*
 * 功能：
 *   关闭 TCP 连接
 */
void tcp_client_close(int sockfd)
{
    if (sockfd >= 0)
    {
        close(sockfd);
    }
}