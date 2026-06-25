#ifndef HTTPS_DASHBOARD_H
#define HTTPS_DASHBOARD_H

#include <string.h>
#include <fcntl.h>
#include "esp_https_server.h"
#include "esp_check.h"
#include "esp_vfs.h"
#include "esp_chip_info.h"
#include "esp_littlefs.h"
#include "cJSON.h"
#include "mdns.h"
#include "freertos/semphr.h"

#if __cplusplus
extern "C"
{
#endif

/** @brief Updates server with new values.
 * @param json_raw_values These are the last values read by the sensor on the following json format
 * {
 *    "tensao":0,
 *    "corrente":0,
 *    "potencia":0
 * }
 * @param json_fft These are the post processed values on the following json format
 * {
 *  "ordens":[],
 *  "tensao_harm":[],
 *  "corrente_harm":[]
 * }
 * @return ESP_OK if successful, otherwise an error code.
**/
esp_err_t update_server_data_values(const char *json_raw_values, const char *json_fft);

/** @brief Initialize and tries to start Wifi.
 * @param partition_name Its the partition with the web page files defined on the partition table
 * @param host_name Name given for the host page of the website
 * @param instance_name Name giver for the instance of the page
 * @param mutex Semaphore made to avoid writing on value being used by update_server_data_values()
 * @return ESP_OK if successful, otherwise an error code.
**/
esp_err_t server_setup(char* partition_name, char* host_name, char* instance_name, SemaphoreHandle_t mutex);

#if __cplusplus
}
#endif

#endif //HTTPS_DASHBOARD_H