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
 * @param partition_name Name of the partition containing the file with the wifi list
 * @param file_name Name of the file with the wifi list example: "wifi_list.json"
 * @details json file must be according to the following example, the file can contain any amount of access points
 * {
 *  "wifi":[
 *      {
 *          "ssid": "Example1",
 *          "password": "Password1"
 *      },
 *      {
 *          "ssid": "Example2",
 *          "password": "Password2"
 *     }
 *  ]
 * }
 * @return ESP_OK if successful, otherwise an error code.
**/
esp_err_t wifi_connect(char* partition_name, char* file_name);

#if __cplusplus
}
#endif

#endif //WIFI_H