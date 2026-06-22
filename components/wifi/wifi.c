#include <stdio.h>
#include "wifi.h"

#define SCAN_LIST_SIZE 20
#define CHANNEL_LIST_SIZE 3

#define DEFAULT_SSID "GABRIEL"
#define DEFAULT_PASSWORD "Master1357@"

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

esp_err_t wifi_init()
{
    ESP_LOGI(TAG, "Initializing wifi component...");
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

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if(sta_netif == NULL){
        ESP_LOGE(TAG, "Failed to create wifi station object: %s", esp_err_to_name(err));
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
        esp_wifi_deinit();
        esp_event_loop_delete_default();
        esp_netif_deinit();
        nvs_flash_deinit();
        return ESP_ERR_NO_MEM;
    }

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

    ESP_LOGI(TAG, "OK");   
    return ESP_OK;
}

esp_err_t wifi_deinit(){
    esp_err_t err = esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failer to unregister IP event: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failer to unregister wifi event: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_wifi_deinit();
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failer to deinit wifi: %s", esp_err_to_name(err));
        return err;
    }
    err = esp_event_loop_delete_default();
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failer to delete event loop: %s", esp_err_to_name(err));
        return err;
    }

    esp_netif_deinit();
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failer to deinit netif: %s", esp_err_to_name(err));
        return err;
    }
    nvs_flash_deinit();
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failer to deinit nvs flash: %s", esp_err_to_name(err));
        return err;
    }
}

esp_err_t littlefs_sta(){

    ESP_LOGI(TAG, "Starting littlefs...");
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/web_files",
        .partition_label = "web_data",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (err == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(err));
        }
        return err;
    }else{
        ESP_LOGI(TAG, "OK");
    }

    return ESP_OK;
}
esp_err_t init_network_functions(uint8_t ssid[32], uint8_t password[64]){

    esp_err_t err = wifi_sta();
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to initialize wifi");
        return err;
    }

    err = littlefs_sta();

    uint16_t number = SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));
    
    ESP_LOGI(TAG, "Allocating memory for scan config...");
    wifi_scan_config_t *scan_config = (wifi_scan_config_t *)calloc(1,sizeof(wifi_scan_config_t));
    if (!scan_config) {
        ESP_LOGE(TAG, "Memory allocation for scan config failed!");
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
        esp_wifi_deinit();
        esp_event_loop_delete_default();
        esp_vfs_littlefs_unregister("web_data");
        esp_netif_deinit();
        nvs_flash_deinit();
        return ESP_ERR_NO_MEM;
    }else{
        ESP_LOGI(TAG, "OK");   
    }

    array_2_channel_bitmap(channel_list, CHANNEL_LIST_SIZE, scan_config);
    ESP_LOGI(TAG, "Starting wifi scan...");
    err = esp_wifi_scan_start(scan_config, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failet to start wifi scan: %s", esp_err_to_name(err));
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
        esp_wifi_deinit();
        esp_event_loop_delete_default();
        esp_vfs_littlefs_unregister("web_data");
        esp_netif_deinit();
        nvs_flash_deinit();
        return err;
    }else{
        ESP_LOGI(TAG, "OK");   
    }
    free(scan_config);

    ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);

    ESP_LOGI(TAG, "Getting number of access points found...");
    err = esp_wifi_scan_get_ap_num(&ap_count);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failet to get number of access points: %s", esp_err_to_name(err));
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
        esp_wifi_deinit();
        esp_event_loop_delete_default();
        esp_vfs_littlefs_unregister("web_data");
        esp_netif_deinit();
        nvs_flash_deinit();
        return err;
    }else{
        ESP_LOGI(TAG, "OK");   
    }

    ESP_LOGI(TAG, "Getting access points lists...");
    err = esp_wifi_scan_get_ap_records(&number, ap_info);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failet to get the list of access points: %s", esp_err_to_name(err));
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
        esp_wifi_deinit();
        esp_event_loop_delete_default();
        esp_vfs_littlefs_unregister("web_data");
        esp_netif_deinit();
        nvs_flash_deinit();
        return err;
    }else{
        ESP_LOGI(TAG, "OK");   
    }

    ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);

    wifi_config_t wifi_config = {
        .sta = {
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold.rssi = -127,
            .threshold.authmode = WIFI_AUTH_OPEN,
            .threshold.rssi_5g_adjustment = 0,
        },
    };

    ESP_LOGI(TAG, "Trying to open wifi list file...");
    FILE *f = fopen("/web_files/wifi_list.json", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Error: Unable to open the file.");
        ESP_LOGI(TAG, "Loading default SSID and Password...");
        strcpy((char*)wifi_config.sta.ssid, DEFAULT_SSID);
        strcpy((char*)wifi_config.sta.password, DEFAULT_PASSWORD);
    }else{
        ESP_LOGI(TAG, "OK");

        ESP_LOGI(TAG, "Trying to allocate memory for file buffer...");
        fseek(f, 0, SEEK_END);
        long string_size = ftell(f);
        fseek(f, 0, SEEK_SET);
        char *buffer = malloc(string_size + 1);
        if(!buffer){
            ESP_LOGE(TAG, "Memory allocation failed for file buffer.");
            fclose(f);
            esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
            esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
            esp_wifi_deinit();
            esp_event_loop_delete_default();
            esp_vfs_littlefs_unregister("web_data");
            esp_netif_deinit();
            nvs_flash_deinit();
            return ESP_ERR_NO_MEM;
        }else{
            ESP_LOGI(TAG, "OK");
        }
        
        ESP_LOGI(TAG,"Trying to pass text in file do buffer...");
        int len = fread(buffer, 1, string_size, f);
        if(len != string_size){
            ESP_LOGE(TAG,"Failed to pass text to buffer.");
            fclose(f);
            esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
            esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
            esp_wifi_deinit();
            esp_event_loop_delete_default();
            esp_vfs_littlefs_unregister("web_data");
            esp_netif_deinit();
            nvs_flash_deinit();
            return ESP_ERR_INVALID_RESPONSE;
        }else{
            ESP_LOGI(TAG, "OK");
        }
        fclose(f);

        buffer[len] = '\0';
        
        ESP_LOGI(TAG, "Trying to convert buffer to cJson type...");
        cJSON *json = cJSON_Parse(buffer);
        if (json == NULL) {
            const char *error_ptr = cJSON_GetErrorPtr();
            if (error_ptr != NULL) {
                ESP_LOGE(TAG,"Failed to convert buffer at: %s", error_ptr);
            }
            free(buffer);
            cJSON_Delete(json);
            esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
            esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
            esp_wifi_deinit();
            esp_event_loop_delete_default();
            esp_vfs_littlefs_unregister("web_data");
            esp_netif_deinit();
            nvs_flash_deinit();
            return ESP_ERR_INVALID_RESPONSE;
        }else{
            ESP_LOGI(TAG, "OK");
        }

        free(buffer);

        ESP_LOGI(TAG, "Trying to get wifi list in json file...");
        cJSON *wifi_list = cJSON_GetObjectItem(json,"wifi");
        if(wifi_list == NULL){
            ESP_LOGE(TAG,"Failed to get wifi list.");
            cJSON_Delete(wifi_list);
            cJSON_Delete(json);
            esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
            esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
            esp_wifi_deinit();
            esp_event_loop_delete_default();
            esp_vfs_littlefs_unregister("web_data");
            esp_netif_deinit();
            nvs_flash_deinit();
            return ESP_ERR_INVALID_RESPONSE;
        }else{
            ESP_LOGI(TAG, "OK");
        }

        ESP_LOGI(TAG,"Iterating over wifi ap and wifi list");
        ESP_LOGI(TAG,"____________________________________");
        bool achouWifi = false;
        for (int i = 0; i < number; i++) {

            ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
            ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
            ESP_LOGI(TAG, "Channel \t\t%d", ap_info[i].primary);

            for(int j = 0; j < cJSON_GetArraySize(wifi_list); j++){
                cJSON *wifi_item = cJSON_GetArrayItem(wifi_list, j);
                if(wifi_item == NULL){
                    ESP_LOGE(TAG,"Failed to iterate over wifi list.");
                    cJSON_Delete(json);
                    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
                    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
                    esp_wifi_deinit();
                    esp_event_loop_delete_default();
                    esp_vfs_littlefs_unregister("web_data");
                    esp_netif_deinit();
                    nvs_flash_deinit();
                    return ESP_ERR_INVALID_RESPONSE;
                }

                cJSON *ssid = cJSON_GetObjectItemCaseSensitive(wifi_item, "ssid");
                if (!(cJSON_IsString(ssid) && (ssid->valuestring != NULL))) {
                    ESP_LOGE(TAG,"Failed to get wifi ssid name.");
                    cJSON_Delete(json);
                    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
                    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
                    esp_wifi_deinit();
                    esp_event_loop_delete_default();
                    esp_vfs_littlefs_unregister("web_data");
                    esp_netif_deinit();
                    nvs_flash_deinit();
                    return ESP_ERR_INVALID_RESPONSE;
                }
                cJSON *password = cJSON_GetObjectItemCaseSensitive(wifi_item, "password");
                if (!(cJSON_IsString(password) && (password->valuestring != NULL))) {
                    ESP_LOGE(TAG,"Failed to get wifi password.");
                    cJSON_Delete(json);
                    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
                    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
                    esp_wifi_deinit();
                    esp_event_loop_delete_default();
                    esp_vfs_littlefs_unregister("web_data");
                    esp_netif_deinit();
                    nvs_flash_deinit();
                    return ESP_ERR_INVALID_RESPONSE;
                }

                if(strcmp((char*) ap_info[i].ssid, ssid->valuestring) == 0){
                    achouWifi = true;
                    ESP_LOGI(TAG,"____________________________________");
                    ESP_LOGI(TAG,"Found ssid match!");
                    strcpy((char*)wifi_config.sta.ssid, ssid->valuestring);
                    strcpy((char*)wifi_config.sta.password, password->valuestring);
                    break;
                }

            }
            if(achouWifi){
                break;
            }
        }
        if(!achouWifi){
            ESP_LOGI(TAG,"____________________________________");
            ESP_LOGI(TAG,"Could not find ssid match, loading default configurations");
            strcpy((char*)wifi_config.sta.ssid, DEFAULT_SSID);
            strcpy((char*)wifi_config.sta.password, DEFAULT_PASSWORD);
        }
        cJSON_Delete(json);
    }

    ESP_LOGI(TAG,"Trying to configure wifi sta...");
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
    }else{
        ESP_LOGI(TAG,"OK");
    }

    ESP_LOGI(TAG, "Connecting to wifi...");
    esp_wifi_connect();
    
    return ESP_OK;
}
