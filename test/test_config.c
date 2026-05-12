#include <stdio.h>
#include "config.h"

int main(void)
{
    monitor_config_t cfg;

    /*
     * 加载配置文件
     */
    if (config_load(CONFIG_FILE_PATH, &cfg) != 0)
    {
        printf("config_load failed, use default config\n");

        /*
         * 如果配置文件加载失败，也可以手动设置默认值
         */
        config_set_default(&cfg);
    }

    /*
     * 打印配置内容，检查解析是否正确
     */
    config_print(&cfg);

    return 0;
}