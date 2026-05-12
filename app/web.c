#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "sem.h"
#include "shm.h"

#define WEB_DEFAULT_PORT 8080
#define REQ_BUF_SIZE 1024
#define RESP_BUF_SIZE 8192
#define LOG_FILE_PATH "logs/monitor.log"
#define LOG_LINES 16
#define LOG_LINE_LEN 256

static const char *INDEX_HTML =
"<!doctype html>\n"
"<html lang=\"zh-CN\">\n"
"<head>\n"
"  <meta charset=\"UTF-8\" />\n"
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />\n"
"  <title>Device Monitor Pro</title>\n"
"  <style>\n"
"    :root { color-scheme: dark; }\n"
"    * { box-sizing: border-box; }\n"
"    body { margin: 0; font-family: Inter, Arial, Helvetica, sans-serif; background: radial-gradient(circle at top, #1e293b 0, #0f172a 42%, #020617 100%); color: #e2e8f0; }\n"
"    .wrap { max-width: 1240px; margin: 0 auto; padding: 24px; }\n"
"    .title { display: flex; justify-content: space-between; align-items: baseline; gap: 12px; flex-wrap: wrap; }\n"
"    h1 { margin: 0; font-size: 28px; letter-spacing: .5px; }\n"
"    .sub { color: #94a3b8; font-size: 14px; }\n"
"    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 16px; margin-top: 20px; }\n"
"    .card { background: rgba(15, 23, 42, .86); border: 1px solid rgba(148, 163, 184, .18); border-radius: 18px; padding: 18px; box-shadow: 0 12px 40px rgba(0,0,0,.25); backdrop-filter: blur(10px); }\n"
"    .card-title { display: flex; justify-content: space-between; align-items: center; gap: 12px; margin-bottom: 10px; }\n"
"    .label { color: #94a3b8; font-size: 13px; }\n"
"    .value { font-size: 32px; font-weight: 800; line-height: 1.1; }\n"
"    .unit { font-size: 14px; color: #94a3b8; margin-left: 6px; }\n"
"    .progress { height: 10px; border-radius: 999px; background: #1f2937; overflow: hidden; margin-top: 12px; }\n"
"    .bar { height: 100%; border-radius: inherit; background: linear-gradient(90deg, #22c55e, #eab308, #ef4444); width: 0%; transition: width .25s ease; }\n"
"    .status-row { display: grid; grid-template-columns: repeat(auto-fit, minmax(260px, 1fr)); gap: 16px; margin-top: 16px; }\n"
"    .mono { font-family: Consolas, Monaco, monospace; }\n"
"    .ok { color: #22c55e; } .warn { color: #f59e0b; } .bad { color: #ef4444; }\n"
"    .log-box { margin-top: 12px; background: #020617; border-radius: 14px; padding: 14px; min-height: 220px; max-height: 320px; overflow: auto; border: 1px solid rgba(148, 163, 184, .14); }\n"
"    .log-line { font-size: 12px; line-height: 1.6; white-space: pre-wrap; color: #cbd5e1; }\n"
"    .footer { margin-top: 16px; color: #64748b; font-size: 12px; }\n"
"  </style>\n"
"</head>\n"
"<body>\n"
"  <div class=\"wrap\">\n"
"    <div class=\"title\">\n"
"      <h1>Device Monitor Pro</h1>\n"
"      <div class=\"sub\">最小可用网页监控界面</div>\n"
"    </div>\n"
"    <div class=\"grid\">\n"
"      <div class=\"card\"><div class=\"card-title\"><div class=\"label\">CPU</div><div class=\"mono\" id=\"cpuState\">--</div></div><div class=\"value\" id=\"cpu\">--<span class=\"unit\">%</span></div><div class=\"progress\"><div class=\"bar\" id=\"cpuBar\"></div></div></div>\n"
"      <div class=\"card\"><div class=\"card-title\"><div class=\"label\">内存</div><div class=\"mono\" id=\"memState\">--</div></div><div class=\"value\" id=\"mem\">--<span class=\"unit\">%</span></div><div class=\"progress\"><div class=\"bar\" id=\"memBar\"></div></div></div>\n"
"      <div class=\"card\"><div class=\"card-title\"><div class=\"label\">磁盘</div><div class=\"mono\" id=\"diskState\">--</div></div><div class=\"value\" id=\"disk\">--<span class=\"unit\">%</span></div><div class=\"progress\"><div class=\"bar\" id=\"diskBar\"></div></div></div>\n"
"      <div class=\"card\"><div class=\"card-title\"><div class=\"label\">温度</div><div class=\"mono\" id=\"tempState\">--</div></div><div class=\"value\" id=\"temp\">--<span class=\"unit\">°C</span></div><div class=\"sub\">acpitz_1 / acpitz_2 / cpu</div></div>\n"
"      <div class=\"card\"><div class=\"card-title\"><div class=\"label\">RX</div><div class=\"mono\" id=\"rxState\">--</div></div><div class=\"value\" id=\"rx\">--<span class=\"unit\">KB/s</span></div></div>\n"
"      <div class=\"card\"><div class=\"card-title\"><div class=\"label\">TX</div><div class=\"mono\" id=\"txState\">--</div></div><div class=\"value\" id=\"tx\">--<span class=\"unit\">KB/s</span></div></div>\n"
"    </div>\n"
"    <div class=\"status-row\">\n"
"      <div class=\"card\">\n"
"        <div class=\"card-title\"><div class=\"label\">系统状态</div><div class=\"mono\" id=\"runState\">--</div></div>\n"
"        <div class=\"sub\">共享内存中的最新值</div>\n"
"        <div style=\"margin-top:12px\" class=\"mono\" id=\"updated\">--</div>\n"
"      </div>\n"
"      <div class=\"card\">\n"
"        <div class=\"card-title\"><div class=\"label\">最近日志</div><div class=\"mono\" id=\"logState\">--</div></div>\n"
"        <div class=\"log-box\" id=\"logs\"></div>\n"
"      </div>\n"
"    </div>\n"
"    <div class=\"footer\">接口：<span class=\"mono\">/api/status</span> 与 <span class=\"mono\">/api/logs</span></div>\n"
"  </div>\n"
"  <script>\n"
"    function setBar(id, v) { document.getElementById(id).style.width = Math.max(0, Math.min(100, v)) + '%'; }\n"
"    function setState(id, v) { document.getElementById(id).textContent = v; }\n"
"    async function refreshStatus() {\n"
"      try {\n"
"        const r = await fetch('/api/status');\n"
"        const d = await r.json();\n"
"        document.getElementById('cpu').innerHTML = d.cpu_usage.toFixed(1) + '<span class=\"unit\">%</span>';\n"
"        document.getElementById('mem').innerHTML = d.mem_usage.toFixed(1) + '<span class=\"unit\">%</span>';\n"
"        document.getElementById('disk').innerHTML = d.disk_usage.toFixed(1) + '<span class=\"unit\">%</span>';\n"
"        document.getElementById('temp').innerHTML = d.temp_cpu.toFixed(1) + '<span class=\"unit\">°C</span>';\n"
"        document.getElementById('rx').innerHTML = d.rx_kbps.toFixed(1) + '<span class=\"unit\">KB/s</span>';\n"
"        document.getElementById('tx').innerHTML = d.tx_kbps.toFixed(1) + '<span class=\"unit\">KB/s</span>';\n"
"        setState('cpuState', d.cpu_usage > 80 ? 'HIGH' : 'NORMAL');\n"
"        setState('memState', d.mem_usage > 80 ? 'HIGH' : 'NORMAL');\n"
"        setState('diskState', d.disk_usage > 80 ? 'HIGH' : 'NORMAL');\n"
"        setState('tempState', d.temp_cpu > 70 ? 'WARN' : 'NORMAL');\n"
"        setState('rxState', 'LIVE');\n"
"        setState('txState', 'LIVE');\n"
"        document.getElementById('runState').textContent = d.running ? 'RUNNING' : 'STOPPED';\n"
"        document.getElementById('runState').className = 'mono ' + (d.running ? 'ok' : 'bad');\n"
"        document.getElementById('updated').textContent = '更新时间：' + new Date().toLocaleString();\n"
"        setBar('cpuBar', d.cpu_usage);\n"
"        setBar('memBar', d.mem_usage);\n"
"        setBar('diskBar', d.disk_usage);\n"
"      } catch (e) {\n"
"        document.getElementById('runState').textContent = 'API ERROR';\n"
"        document.getElementById('runState').className = 'mono warn';\n"
"      }\n"
"    }\n"
"    async function refreshLogs() {\n"
"      try {\n"
"        const r = await fetch('/api/logs');\n"
"        const d = await r.json();\n"
"        const box = document.getElementById('logs');\n"
"        box.innerHTML = '';\n"
"        (d.lines || []).forEach((line) => {\n"
"          const p = document.createElement('div');\n"
"          p.className = 'log-line';\n"
"          p.textContent = line;\n"
"          box.appendChild(p);\n"
"        });\n"
"        document.getElementById('logState').textContent = (d.lines || []).length ? 'OK' : 'EMPTY';\n"
"      } catch (e) {\n"
"        document.getElementById('logState').textContent = 'ERROR';\n"
"      }\n"
"    }\n"
"    refreshStatus();\n"
"    refreshLogs();\n"
"    setInterval(refreshStatus, 1000);\n"
"    setInterval(refreshLogs, 3000);\n"
"  </script>\n"
"</body>\n"
"</html>\n";

static int send_all(int fd, const char *buf, size_t len)
{
    size_t sent = 0;
    while (sent < len)
    {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0)
        {
            return -1;
        }
        sent += (size_t)n;
    }
    return 0;
}

static int send_response(int client_fd, const char *status, const char *content_type, const char *body)
{
    char resp[RESP_BUF_SIZE];
    int body_len = (int)strlen(body);
    int len = snprintf(resp, sizeof(resp),
                       "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s",
                       status, content_type, body_len, body);
    if (len < 0 || len >= (int)sizeof(resp))
    {
        return -1;
    }
    return send_all(client_fd, resp, (size_t)len);
}

static int send_json_status(int client_fd, system_status_t *status, sem_t *sem)
{
    char body[RESP_BUF_SIZE];
    system_status_t snapshot;

    sem_lock(sem);
    snapshot = *status;
    sem_unlock(sem);

    snprintf(body, sizeof(body),
             "{\"running\":%d,\"cpu_usage\":%.1f,\"mem_usage\":%.1f,\"temp_acpitz_1\":%.1f,\"temp_acpitz_2\":%.1f,\"temp_cpu\":%.1f,\"disk_usage\":%.1f,\"rx_kbps\":%.1f,\"tx_kbps\":%.1f}",
             snapshot.running,
             snapshot.cpu_usage,
             snapshot.mem_usage,
             snapshot.temp_acpitz_1,
             snapshot.temp_acpitz_2,
             snapshot.temp_cpu,
             snapshot.disk_usage,
             snapshot.rx_kbps,
             snapshot.tx_kbps);

    return send_response(client_fd, "200 OK", "application/json; charset=utf-8", body);
}

static int send_json_logs(int client_fd)
{
    FILE *fp = fopen(LOG_FILE_PATH, "r");
    char lines[LOG_LINES][LOG_LINE_LEN];
    char line[LOG_LINE_LEN];
    int count = 0;
    int idx = 0;
    char body[RESP_BUF_SIZE];

    if (fp != NULL)
    {
        while (fgets(line, sizeof(line), fp) != NULL)
        {
            snprintf(lines[idx], sizeof(lines[idx]), "%s", line);
            size_t len = strlen(lines[idx]);
            while (len > 0 && (lines[idx][len - 1] == '\n' || lines[idx][len - 1] == '\r'))
            {
                lines[idx][len - 1] = '\0';
                len--;
            }
            idx = (idx + 1) % LOG_LINES;
            if (count < LOG_LINES)
            {
                count++;
            }
        }
        fclose(fp);
    }

    snprintf(body, sizeof(body), "{\"lines\":[");
    for (int i = 0; i < count; i++)
    {
        int pos = (idx - count + i + LOG_LINES) % LOG_LINES;
        char escaped[LOG_LINE_LEN * 2];
        int j = 0;
        for (size_t k = 0; lines[pos][k] != '\0' && j < (int)sizeof(escaped) - 2; k++)
        {
            if (lines[pos][k] == '"' || lines[pos][k] == '\\')
            {
                escaped[j++] = '\\';
            }
            escaped[j++] = lines[pos][k];
        }
        escaped[j] = '\0';

        strncat(body, "\"", sizeof(body) - strlen(body) - 1);
        strncat(body, escaped, sizeof(body) - strlen(body) - 1);
        strncat(body, "\"", sizeof(body) - strlen(body) - 1);
        if (i != count - 1)
        {
            strncat(body, ",", sizeof(body) - strlen(body) - 1);
        }
    }
    strncat(body, "]}", sizeof(body) - strlen(body) - 1);

    return send_response(client_fd, "200 OK", "application/json; charset=utf-8", body);
}

int main(int argc, char *argv[])
{
    int port = WEB_DEFAULT_PORT;
    int server_fd = -1;
    system_status_t *status = NULL;
    sem_t *sem = NULL;

    if (argc > 1)
    {
        port = atoi(argv[1]);
        if (port <= 0)
        {
            port = WEB_DEFAULT_PORT;
        }
    }

    status = shm_open_existing();
    if (status == NULL)
    {
        perror("[web] shm_open_existing failed");
        return 1;
    }

    sem = sem_open_existing();
    if (sem == NULL)
    {
        shm_detach(status);
        perror("[web] sem_open_existing failed");
        return 1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("[web] socket failed");
        sem_close_handle(sem);
        shm_detach(status);
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("[web] bind failed");
        close(server_fd);
        sem_close_handle(sem);
        shm_detach(status);
        return 1;
    }

    if (listen(server_fd, 16) < 0)
    {
        perror("[web] listen failed");
        close(server_fd);
        sem_close_handle(sem);
        shm_detach(status);
        return 1;
    }

    printf("[web] server started: http://127.0.0.1:%d\n", port);

    while (1)
    {
        int client_fd;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        char req[REQ_BUF_SIZE];
        ssize_t n;

        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            perror("[web] accept failed");
            continue;
        }

        n = recv(client_fd, req, sizeof(req) - 1, 0);
        if (n <= 0)
        {
            close(client_fd);
            continue;
        }
        req[n] = '\0';

        if (strncmp(req, "GET /api/status", 15) == 0)
        {
            send_json_status(client_fd, status, sem);
        }
        else if (strncmp(req, "GET /api/logs", 13) == 0)
        {
            send_json_logs(client_fd);
        }
        else
        {
            send_response(client_fd, "200 OK", "text/html; charset=utf-8", INDEX_HTML);
        }

        close(client_fd);
    }

    close(server_fd);
    sem_close_handle(sem);
    shm_detach(status);
    return 0;
}
