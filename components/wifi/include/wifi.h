#ifndef WIFI_H
#define WIFI_H

#include "esp_wifi.h"
#include <nvs_flash.h>
#include "esp_log.h"
#include "esp_littlefs.h"
#include "cJSON.h"
#include "mdns.h"

#if __cplusplus
extern "C"
{
#endif

/** @brief Initialize and tries to start Wifi.
 * @param ssid is the ssid of the network trying to be conected to.
 * @param password is the password of the network trying to be conected to.
 * @return ESP_OK if successful, otherwise an error code.
**/
esp_err_t init_network_functions(
    uint8_t ssid[32], 
    uint8_t password[64]);

#if __cplusplus
}
#endif

#endif //WIFI_H