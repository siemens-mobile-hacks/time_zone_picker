#include <swilib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define WORLDCLOCK_PD_ID 0x220C

int ReadCurrentCity() {
    char key[32];
    char value[32];
    uint32_t len;

    len = 32;
    sprintf(key, "%s", "current_city");
    if (ReadValueFromPDFile(WORLDCLOCK_PD_ID, key, value, &len) == 0) {
        return atoi(value);
    }
    return -1;
}

int WriteCurrentCity(int city_id) {
    char value[32];
    sprintf(value, "%d", city_id);
    return (WriteValueToPDFile(WORLDCLOCK_PD_ID, "current_city", value, strlen(value)) == 0);
}
