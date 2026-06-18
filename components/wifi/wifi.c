#include <stdio.h>
#include "wifi.h"

#define DEFAULT_SCAN_LIST_SIZE 20
#define CHANNEL_LIST_SIZE 3
static uint8_t channel_list[CHANNEL_LIST_SIZE] = {1, 6, 11};

static const char *TAG = "WIFI";

static void array_2_channel_bitmap(const uint8_t channel_list[], const uint8_t channel_list_size, wifi_scan_config_t *scan_config) {

    for(uint8_t i = 0; i < channel_list_size; i++) {
        uint8_t channel = channel_list[i];
        scan_config->channel_bitmap.ghz_2_channels |= (1 << channel);
    }
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

esp_err_t wifi_sta(uint8_t ssid[32], uint8_t password[64]){

    ESP_LOGI(TAG, "Initializing nvs flash...");
    esp_err_t err = nvs_flash_init();
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to init nvs flash: %s", esp_err_to_name(err));
        
        return err;
    }else{
        ESP_LOGI(TAG, "OK");
    }

    ESP_LOGI(TAG, "Initializing netif...");
    err = esp_netif_init();
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to init Netif: %s", esp_err_to_name(err));
        nvs_flash_deinit();
        return err;
    }else{
        ESP_LOGI(TAG, "OK");
    }

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/web_files",
        .partition_label = "web_data",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    ESP_LOGI(TAG, "Initializing littlefs...");
    err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (err == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(err));
        }
        esp_netif_deinit();
        nvs_flash_deinit();
        return err;
    }else{
        ESP_LOGI(TAG, "OK");
    }

    err = esp_event_loop_create_default();
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(err));
        esp_vfs_littlefs_unregister("web_data");
        esp_netif_deinit();
        nvs_flash_deinit();
        return err;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to init wifi: %s", esp_err_to_name(err));
        esp_event_loop_delete_default();
        esp_vfs_littlefs_unregister("web_data");
        esp_netif_deinit();
        nvs_flash_deinit();
        return err;
    }

    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to register wifi event handler: %s", esp_err_to_name(err));
        esp_wifi_deinit();
        esp_event_loop_delete_default();
        esp_vfs_littlefs_unregister("web_data");
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
        esp_vfs_littlefs_unregister("web_data");
        esp_netif_deinit();
        nvs_flash_deinit();
        return err;
    }
    // Initialize default station as network interface instance (esp-netif)
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);


    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to set wifi mode: %s", esp_err_to_name(err));
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
        esp_wifi_deinit();
        esp_event_loop_delete_default();
        esp_vfs_littlefs_unregister("web_data");
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
        esp_vfs_littlefs_unregister("web_data");
        esp_netif_deinit();
        nvs_flash_deinit();
        return err;
    }

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));
    
    wifi_scan_config_t *scan_config = (wifi_scan_config_t *)calloc(1,sizeof(wifi_scan_config_t));
    if (!scan_config) {
        ESP_LOGE(TAG, "Memory Allocation for scan config failed!");
        return ESP_ERR_NO_MEM;
    }
    array_2_channel_bitmap(channel_list, CHANNEL_LIST_SIZE, scan_config);
    esp_wifi_scan_start(scan_config, true);
    free(scan_config);

    ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);
    
    for (int i = 0; i < number; i++) {
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
        ESP_LOGI(TAG, "Channel \t\t%d", ap_info[i].primary);
    }

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
    
    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to set wifi config: %s", esp_err_to_name(err));
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
        esp_wifi_deinit();
        esp_event_loop_delete_default();
        esp_vfs_littlefs_unregister("web_data");
        esp_netif_deinit();
        nvs_flash_deinit();
        return err;
    }

    ESP_LOGI(TAG, "Wifi Started successfully!");
    return ESP_OK;
}
