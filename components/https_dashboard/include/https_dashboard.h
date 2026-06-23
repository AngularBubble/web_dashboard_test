#ifndef HTTPS_DASHBOARD_H
#define HTTPS_DASHBOARD_H

#include "esp_err.h"

#if __cplusplus
extern "C"
{
#endif

/** @brief Initialize and tries to start Wifi.
 * @return ESP_OK if successful, otherwise an error code.
**/
esp_err_t server_setup();

#if __cplusplus
}
#endif

#endif //HTTPS_DASHBOARD_H