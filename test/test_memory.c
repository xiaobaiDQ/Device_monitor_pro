#include <stdio.h>
#include "memory.h"

int main(void)
{
    float mem = get_memory_usage();

    if (mem < 0.0f)
    {
        printf("get_memory_usage failed\n");
        return 1;
    }

    printf("memory usage = %.2f%%\n", mem);
    return 0;
}