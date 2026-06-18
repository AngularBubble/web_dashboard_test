#include <stdio.h>
#include "wifi.h"

static const char *TAG = "WIFI";

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

esp_err_t wifi_sta(uint8_t ssid[32], uint8_t password[64]){

    ESP_LOGI(TAG, "Starting Wifi...");
    esp_err_t err = nvs_flash_init();
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to init nvs flash: %s", esp_err_to_name(err));
        
        return err;
    }

    err = esp_netif_init();
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to init Netif: %s", esp_err_to_name(err));
        nvs_flash_deinit();
        return err;
    }

    err = esp_event_loop_create_default();
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(err));
        esp_netif_deinit();
        nvs_flash_deinit();
        return err;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to init wifi: %s", esp_err_to_name(err));
        esp_event_loop_delete_default();
        esp_netif_deinit();
        nvs_flash_deinit();
        return err;
    }

    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to register wifi event handler: %s", esp_err_to_name(err));
        esp_wifi_deinit();
        esp_event_loop_delete_default();
        esp_netif_deinit();
        nvs_flash_deinit();
        return err;
    }
    err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to register ip event handler: %s", esp_err_to_name(err));
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
        esp_wifi_deinit();
        esp_event_loop_delete_default();
        esp_netif_deinit();
        nvs_flash_deinit();
        return err;
    }
    // Initialize default station as network interface instance (esp-netif)
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // Initialize and start WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold.rssi = -127,
            .threshold.authmode = WIFI_AUTH_OPEN,
            .threshold.rssi_5g_adjustment = 0,
        },
    };
    strcpy((char*)wifi_config.sta.ssid, (char*)ssid);
    strcpy((char*)wifi_config.sta.password, (char*)password);

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to set wifi mode: %s", esp_err_to_name(err));
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
        esp_wifi_deinit();
        esp_event_loop_delete_default();
        esp_netif_deinit();
        nvs_flash_deinit();
        return err;
    }
    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to set wifi config: %s", esp_err_to_name(err));
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
        esp_wifi_deinit();
        esp_event_loop_delete_default();
        esp_netif_deinit();
        nvs_flash_deinit();
        return err;
    }
    err = esp_wifi_start();
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to start wifi: %s", esp_err_to_name(err));
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
        esp_wifi_deinit();
        esp_event_loop_delete_default();
        esp_netif_deinit();
        nvs_flash_deinit();
        return err;
    }

    ESP_LOGI(TAG, "Wifi Started successfully!");
    return ESP_OK;
}
