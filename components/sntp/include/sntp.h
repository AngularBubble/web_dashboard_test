#ifndef SNTP_H
#define SNTP_H

#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"

#if __cplusplus
extern "C"
{
#endif

esp_err_t sntp_comp_init(char* server_name);

#if __cplusplus
}
#endif

#endif //SNTP_H