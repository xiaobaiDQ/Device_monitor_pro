#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define SERVER_PORT 8888
#define RECV_BUF_SIZE 512
#define CMD_BUF_SIZE 256

/*
 * 功能：
 *   创建、绑定并监听 TCP 服务端 socket
 *
 * 返回值：
 *   成功：返回 listen_fd
 *   失败：返回 -1
 */
static int create_server_socket(void)
{
    int listen_fd;
    struct sockaddr_in server_addr;

    /*
     * 创建 TCP socket
     */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1)
    {
        perror("socket failed");
        return -1;
    }

    /*
     * 设置端口复用
     * 作用：
     *   防止服务端刚退出后，端口短时间内被占用导致 bind failed
     */
    int opt = 1;
    if (setsockopt(listen_fd,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &opt,
                   sizeof(opt)) == -1)
    {
        perror("setsockopt failed");
        close(listen_fd);
        return -1;
    }

    /*
     * 初始化服务器地址结构体
     */
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;              /* IPv4 */
    server_addr.sin_port = htons(SERVER_PORT);     /* 端口转网络字节序 */
    server_addr.sin_addr.s_addr = INADDR_ANY;      /* 监听所有网卡 */

    /*
     * 绑定 IP 和端口
     */
    if (bind(listen_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) == -1)
    {
        perror("bind failed");
        close(listen_fd);
        return -1;
    }

    /*
     * 开始监听
     */
    if (listen(listen_fd, 5) == -1)
    {
        perror("listen failed");
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}

/*
 * 功能：
 *   打印命令提示符
 */
static void print_command_prompt(void)
{
    printf("[server] command> ");
    fflush(stdout);
}

/*
 * 功能：
 *   处理已经连接上的客户端
 *
 * 返回：
 *   0：客户端断开，回到 accept 等待新连接
 *  -1：服务端异常
 */
static int handle_client(int client_fd)
{
    char recv_buf[RECV_BUF_SIZE];
    char cmd_buf[CMD_BUF_SIZE];

    printf("[server] client connected\n");
    printf("[server] input command:\n");
    printf("  GET_STATUS\n");
    printf("  SET_INTERVAL 2\n");
    printf("  STOP\n");
    print_command_prompt();

    /*
     * 当前客户端连接期间，循环监听：
     *   1. client_fd：设备端上传的数据
     *   2. STDIN_FILENO：键盘输入的命令
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
            return -1;
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

                /*
                 * 前面加换行，避免和用户正在输入的命令挤在一行
                 */
                printf("\n[server] recv: %s", recv_buf);
                print_command_prompt();
            }
            else if (n == 0)
            {
                /*
                 * n == 0 表示客户端正常断开连接
                 * 这里不退出整个服务端，而是返回外层重新 accept
                 */
                printf("\n[server] client disconnected\n");
                return 0;
            }
            else
            {
                perror("recv failed");
                return 0;
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
                printf("\n[server] stdin closed\n");
                return -1;
            }

            /*
             * 把命令发送给客户端
             */
            if (send(client_fd, cmd_buf, strlen(cmd_buf), 0) == -1)
            {
                perror("send command failed");

                /*
                 * 发送失败通常说明客户端断开
                 * 返回外层重新 accept
                 */
                return 0;
            }

            printf("[server] send command: %s", cmd_buf);
            print_command_prompt();
        }
    }
}

int main(void)
{
    int listen_fd;

    /*
     * 创建服务端监听 socket
     */
    listen_fd = create_server_socket();
    if (listen_fd == -1)
    {
        return 1;
    }

    printf("[server] listen on port %d\n", SERVER_PORT);

    /*
     * 外层循环：
     *   服务端一直运行
     *   每次 accept 一个客户端
     *   客户端断开后继续 accept 下一个客户端
     */
    while (1)
    {
        int client_fd;
        struct sockaddr_in client_addr;
        socklen_t client_len;

        client_len = sizeof(client_addr);

        printf("[server] waiting for client...\n");

        /*
         * 等待客户端连接
         */
        client_fd = accept(listen_fd,
                           (struct sockaddr *)&client_addr,
                           &client_len);

        if (client_fd == -1)
        {
            perror("accept failed");
            continue;
        }

        /*
         * 处理当前客户端
         */
        if (handle_client(client_fd) < 0)
        {
            /*
             * 服务端自身异常，退出程序
             */
            close(client_fd);
            break;
        }

        /*
         * 当前客户端断开后，关闭 client_fd
         * 然后回到 while(1) 继续 accept
         */
        close(client_fd);

        printf("[server] ready to accept next client\n");
    }

    close(listen_fd);

    return 0;
}