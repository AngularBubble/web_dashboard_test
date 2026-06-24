#include <stdio.h>
#include "wifi.h"
#include "https_dashboard.h"

void app_main(void)
{
    wifi_connect();
    server_setup();
}
