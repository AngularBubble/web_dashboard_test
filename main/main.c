#include <stdio.h>
#include "wifi.h"

void app_main(void)
{
    uint8_t ssid[32] = "GABRIEL";
    uint8_t password[64] = "Master1357@";
    wifi_sta(ssid, password);
}
