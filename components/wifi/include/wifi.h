#ifndef WIFI_H
#define WIFI_H

#include "esp_wifi.h"
#include <nvs_flash.h>
#include "esp_log.h"
#include "esp_littlefs.h"
#include "cJSON.h"

#if __cplusplus
extern "C"
{
#endif

/** @brief Initialize and tries to connect to Wifi AP using wifi_list file.
 * @return ESP_OK if successful, otherwise an error code.
**/
esp_err_t wifi_connect();

#if __cplusplus
}
#endif

#endif //WIFI_H