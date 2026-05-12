#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define SERVER_PORT 8888
#define RECV_BUF_SIZE 512
#define CMD_BUF_SIZE 256

int main(void)
{
    int listen_fd;
    int client_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len;

    char recv_buf[RECV_BUF_SIZE];
    char cmd_buf[CMD_BUF_SIZE];

    /*
     * 创建 TCP 监听 socket
     */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1)
    {
        perror("socket failed");
        return 1;
    }

    /*
     * 端口复用，避免刚退出程序后端口短时间不可用
     */
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /*
     * 配置服务器地址
     */
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /*
     * 绑定端口
     */
    if (bind(listen_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) == -1)
    {
        perror("bind failed");
        close(listen_fd);
        return 1;
    }

    /*
     * 开始监听
     */
    if (listen(listen_fd, 5) == -1)
    {
        perror("listen failed");
        close(listen_fd);
        return 1;
    }

    printf("[server] listen on port %d\n", SERVER_PORT);

    /*
     * 等待一个客户端连接
     */
    client_len = sizeof(client_addr);
    client_fd = accept(listen_fd,
                       (struct sockaddr *)&client_addr,
                       &client_len);

    if (client_fd == -1)
    {
        perror("accept failed");
        close(listen_fd);
        return 1;
    }

    printf("[server] client connected\n");
    printf("[server] input command:\n");
    printf("  GET_STATUS\n");
    printf("  SET_INTERVAL 2\n");
    printf("  STOP\n");

    /*
     * 主循环：
     *   同时监听 socket 和键盘输入
     */
    while (1)
    {
        fd_set readfds;
        int maxfd;
        int ret;

        FD_ZERO(&readfds);

        /*
         * 监听客户端 socket
         */
        FD_SET(client_fd, &readfds);

        /*
         * 监听标准输入，也就是键盘输入
         */
        FD_SET(STDIN_FILENO, &readfds);

        maxfd = client_fd > STDIN_FILENO ? client_fd : STDIN_FILENO;

        /*
         * select 阻塞等待：
         *   - 客户端上传数据
         *   - 用户输入命令
         */
        ret = select(maxfd + 1, &readfds, NULL, NULL, NULL);

        if (ret < 0)
        {
            perror("select failed");
            break;
        }

        /*
         * 处理客户端上传的数据
         */
        if (FD_ISSET(client_fd, &readfds))
        {
            ssize_t n;

            memset(recv_buf, 0, sizeof(recv_buf));

            n = recv(client_fd, recv_buf, sizeof(recv_buf) - 1, 0);

            if (n > 0)
            {
                recv_buf[n] = '\0';
                printf("[server] recv: %s", recv_buf);
            }
            else if (n == 0)
            {
                printf("[server] client disconnected\n");
                break;
            }
            else
            {
                perror("recv failed");
                break;
            }
        }

        /*
         * 处理键盘输入命令
         */
        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            memset(cmd_buf, 0, sizeof(cmd_buf));

            /*
             * 从终端读取一行命令
             */
            if (fgets(cmd_buf, sizeof(cmd_buf), stdin) == NULL)
            {
                printf("[server] stdin closed\n");
                break;
            }

            /*
             * 把命令发送给客户端
             */
            if (send(client_fd, cmd_buf, strlen(cmd_buf), 0) == -1)
            {
                perror("send command failed");
                break;
            }

            printf("[server] send command: %s", cmd_buf);
        }
    }

    close(client_fd);
    close(listen_fd);

    return 0;
}