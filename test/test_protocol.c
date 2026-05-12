#include <stdio.h>

#include "common.h"
#include "protocol.h"

static void test_build_status(void)
{
    system_status_t status;
    char buf[256];

    status.cpu_usage = 12.3f;
    status.mem_usage = 45.6f;
    status.temp_acpitz_1 = 50.1f;
    status.temp_acpitz_2 = 48.2f;
    status.temp_cpu = 55.0f;
    status.running = 1;

    if (protocol_build_status_json(&status, buf, sizeof(buf)) == 0)
    {
        printf("status json: %s", buf);
    }
    else
    {
        printf("protocol_build_status_json failed\n");
    }
}

static void test_build_ack(void)
{
    system_status_t status;
    char buf[256];

    status.cpu_usage = 20.0f;
    status.mem_usage = 40.0f;
    status.temp_acpitz_1 = 55.0f;
    status.temp_acpitz_2 = 52.0f;
    status.temp_cpu = 60.0f;
    status.running = 1;

    if (protocol_build_ack_status(&status, buf, sizeof(buf)) == 0)
    {
        printf("ack status: %s", buf);
    }

    if (protocol_build_ack_interval(5, buf, sizeof(buf)) == 0)
    {
        printf("ack interval: %s", buf);
    }

    if (protocol_build_ack_stop(buf, sizeof(buf)) == 0)
    {
        printf("ack stop: %s", buf);
    }

    if (protocol_build_error("unknown command", buf, sizeof(buf)) == 0)
    {
        printf("error: %s", buf);
    }
}

static void test_parse_command(const char *cmd_str)
{
    command_t cmd;

    if (protocol_parse_command(cmd_str, &cmd) != 0)
    {
        printf("parse failed: %s\n", cmd_str);
        return;
    }

    printf("raw=%s, type=%d, interval=%d\n",
           cmd.raw,
           cmd.type,
           cmd.interval);
}

int main(void)
{
    printf("===== test build status =====\n");
    test_build_status();

    printf("\n===== test build ack/error =====\n");
    test_build_ack();

    printf("\n===== test parse command =====\n");
    test_parse_command("GET_STATUS\n");
    test_parse_command("SET_INTERVAL 5\n");
    test_parse_command("SET_INTERVAL abc\n");
    test_parse_command("STOP\n");
    test_parse_command("ABC\n");

    return 0;
}