#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

/*
 * 功能：
 *   连接 TCP 服务器
 *
 * 参数：
 *   ip   ：服务器 IP 地址，例如 "127.0.0.1"
 *   port ：服务器端口，例如 8888
 *
 * 返回值：
 *   成功：返回 socket fd
 *   失败：返回 -1
 */
int tcp_client_connect(const char *ip, int port);

/*
 * 功能：
 *   发送字符串数据
 *
 * 参数：
 *   sockfd：socket 文件描述符
 *   data  ：要发送的字符串
 *
 * 返回值：
 *   成功：0
 *   失败：-1
 */
int tcp_client_send(int sockfd, const char *data);

/*
 * 功能：
 *   带超时接收服务器下发的数据
 *
 * 参数：
 *   sockfd      ：socket 文件描述符
 *   buf         ：接收缓冲区
 *   size        ：缓冲区大小
 *   timeout_sec ：超时时间，单位秒
 *
 * 返回值：
 *   >0 ：实际接收到的字节数
 *    0 ：超时，没有收到数据
 *   -1 ：接收失败或连接断开
 *
 * 说明：
 *   这个函数内部使用 select()，不会一直阻塞。
 */
int tcp_client_recv_timeout(int sockfd, char *buf, int size, int timeout_sec);

/*
 * 功能：
 *   关闭 TCP 连接
 */
void tcp_client_close(int sockfd);

#endif