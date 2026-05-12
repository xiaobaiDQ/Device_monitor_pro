#include <stdio.h>
#include "temp.h"

int main(void)
{
    float temp_acpitz_1 = get_temperature(0);
    float temp_acpitz_2 = get_temperature(1);
    float temp_cpu = get_temperature(2);
    if (temp_acpitz_1 < 0.0f)
    {
        printf("get_temperature (acpitz_1) failed\n");
        return 1;
    }

    if (temp_acpitz_2 < 0.0f)
    {
        printf("get_temperature (acpitz_2) failed\n");
        return 1;
    }
    if (temp_cpu < 0.0f)
    {
        printf("get_temperature (cpu) failed\n");
        return 1;
    }

    printf("temperature (acpitz_1) = %.2f℃\n", temp_acpitz_1);
    printf("temperature (acpitz_2) = %.2f℃\n", temp_acpitz_2);
    printf("temperature (cpu) = %.2f℃\n", temp_cpu);
    return 0;
}