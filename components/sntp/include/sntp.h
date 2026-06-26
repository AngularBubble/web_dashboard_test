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

/** @brief Updates internal timer through SNTP
 * @return None.
**/
void sntp_sync();

/** @brief Initialize and tries to update internal timer with sntp.
 * @param server_name Server used to sync time
 * @return ESP_OK if successful, otherwise an error code.
**/
esp_err_t sntp_comp_init(char* server_name);

#if __cplusplus
}
#endif

#endif //SNTP_H