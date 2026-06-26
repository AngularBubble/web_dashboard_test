#include <stdio.h>
#include "wifi.h"
#include "https_dashboard.h"
#include "sntp.h"
#include "mqtt_comp.h"

#define STACK_SIZE 2048

void task_calculo_energia( void * pvParameters );

static esp_mqtt5_user_property_item_t user_property_arr[] = {
        {"board", "esp32"},
        {"u", "user"},
        {"p", "password"}
    };

#define USE_PROPERTY_ARR_SIZE   sizeof(user_property_arr)/sizeof(esp_mqtt5_user_property_item_t)

void app_main(void)
{
    static uint8_t ucParameterToPass;
    TaskHandle_t xHandle = NULL;

    wifi_connect("web_data", "wifi_list.json");
    server_setup("www","power_analiser","dashboard do medidor e analizador de potencia");
    sntp_comp_init("pool.ntp.org");
    mqtt_comp_init(user_property_arr, USE_PROPERTY_ARR_SIZE, "mqtt://test.mosquitto.org", "/test/response", "/topic/will", "i will leave");
    xTaskCreate( task_calculo_energia, "task_calculo_energia", STACK_SIZE, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle );
}

void task_calculo_energia(void *pvParameters) {
     while(1) {
        float v_rms = 221.4;
        float i_rms = 4.35;
        float p_ativa = v_rms * i_rms;

        float tensao_harm_array[] = {100.0, 0.5, 4.2, 0.1, 1.8}; 
        float corrente_harm_array[] = {100.0, 1.1, 12.4, 0.3, 5.2};


        cJSON *root_g = cJSON_CreateObject();
        cJSON_AddNumberToObject(root_g, "tensao", v_rms);
        cJSON_AddNumberToObject(root_g, "corrente", i_rms);
        cJSON_AddNumberToObject(root_g, "potencia", p_ativa);

        char *str_grandezas = cJSON_PrintUnformatted(root_g);

        cJSON *root_f = cJSON_CreateObject();
        
        cJSON *json_arr_v = cJSON_CreateFloatArray(tensao_harm_array, 5);
        cJSON_AddItemToObject(root_f, "tensao_harm", json_arr_v);

        cJSON *json_arr_i = cJSON_CreateFloatArray(corrente_harm_array, 5);
        cJSON_AddItemToObject(root_f, "corrente_harm", json_arr_i);

        char *str_fft = cJSON_PrintUnformatted(root_f);

        esp_err_t err = update_server_data_values(str_grandezas, str_fft);
        if (err != ESP_OK) {
            ESP_LOGW("MEDIDOR", "Falha ao atualizar dados (Servidor ocupado ou travado)");
        }

        cJSON_free(str_grandezas);
        cJSON_Delete(root_g);
        
        cJSON_free(str_fft);
        cJSON_Delete(root_f);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}