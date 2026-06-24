#ifndef HTTPS_DASHBOARD_H
#define HTTPS_DASHBOARD_H

#include <string.h>
#include <fcntl.h>
#include "esp_https_server.h"
#include "esp_check.h"
#include "esp_vfs.h"
#include "esp_littlefs.h"
#include "mdns.h"

#if __cplusplus
extern "C"
{
#endif

/** @brief Initialize and tries to start Wifi.
 * @param partition_name Its the partition with the web page files defined on the partition table
 * @return ESP_OK if successful, otherwise an error code.
**/
esp_err_t server_setup(char* partition_name);

#if __cplusplus
}
#endif

#endif //HTTPS_DASHBOARD_H