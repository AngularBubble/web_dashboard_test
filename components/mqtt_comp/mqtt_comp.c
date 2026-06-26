#include <stdio.h>
#include "mqtt_comp.h"

#define MQTT_BUFFER_SIZE 1024

static const char *TAG = "MQTT5";

static esp_mqtt_client_handle_t client;

static SemaphoreHandle_t mutex = NULL; 

static char last_received_data[MQTT_BUFFER_SIZE];

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static esp_mqtt5_publish_property_config_t publish_property = {
    .payload_format_indicator = 1,
    .message_expiry_interval = 1000,
    .topic_alias = 0,
    .correlation_data = "123456",
    .correlation_data_len = 6,
};

static esp_mqtt5_subscribe_property_config_t subscribe_property = {
    .subscribe_id = 25555,
    .no_local_flag = false,
    .retain_as_published_flag = false,
    .retain_handle = 0,
    .is_share_subscribe = true,
    .share_name = "group1",
};

static esp_mqtt5_unsubscribe_property_config_t unsubscribe_property = {
    .is_share_subscribe = true,
    .share_name = "group1",
};

static esp_mqtt5_disconnect_property_config_t disconnect_property = {
    .session_expiry_interval = 60,
    .disconnect_reason = 0,
};

static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    ESP_LOGD(TAG, "free heap size is %" PRIu32 ", minimum %" PRIu32, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");  
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d, reason code=0x%02x ", event->msg_id, (uint8_t)*event->data);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        esp_mqtt5_client_set_disconnect_property(client, &disconnect_property);
        esp_mqtt5_client_delete_user_property(disconnect_property.user_property);
        disconnect_property.user_property = NULL;
        esp_mqtt_client_disconnect(client);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "payload_format_indicator is %d", event->property->payload_format_indicator);
        ESP_LOGI(TAG, "response_topic is %.*s", event->property->response_topic_len, event->property->response_topic);
        ESP_LOGI(TAG, "correlation_data is %.*s", event->property->correlation_data_len, event->property->correlation_data);
        ESP_LOGI(TAG, "content_type is %.*s", event->property->content_type_len, event->property->content_type);
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
        
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            memcpy(last_received_data, event->data, event->data_len);
            last_received_data[event->data_len] = '\0';
            xSemaphoreGive(mutex);
        }

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        ESP_LOGI(TAG, "MQTT5 return code is %d", event->error_handle->connect_return_code);
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

bool get_last_mqtt_message(char *buffer_destiny, size_t buffer_size) {
    bool possui_mensagem_nova = false;

    // Inicializa o seu buffer de destino como vazio por segurança
    if (buffer_destiny && buffer_size > 0) {
        buffer_destiny[0] = '\0';
    } else {
        return false;
    }

    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        // CORREÇÃO: Em C, se o primeiro caractere for '\0', a string está vazia
        if (last_received_data[0] != '\0') {
            
            // Copia o dado global para o seu buffer de destino de forma estável
            strlcpy(buffer_destiny, last_received_data, buffer_size);
            
            // CORREÇÃO: Limpa o buffer global para que a próxima leitura retorne false
            last_received_data[0] = '\0'; 
            
            possui_mensagem_nova = true;
        }
        xSemaphoreGive(mutex);
    }

    return possui_mensagem_nova;
}

esp_err_t unsubscribe_to_topic(char* unsubscribe_topic){
    esp_err_t err = esp_mqtt5_client_set_unsubscribe_property(client, &unsubscribe_property);
    if(err != ESP_OK){
        ESP_LOGE(TAG,"Failed to set unsubscribe property: %s", esp_err_to_name(err));
        return err;
    }
    int err_sub = esp_mqtt_client_unsubscribe(client, unsubscribe_topic);
    if(err_sub < 0){
        ESP_LOGE(TAG,"Failed to unsubscribe to topic");
        return ESP_FAIL;
    }
    esp_mqtt5_client_delete_user_property(unsubscribe_property.user_property);
    unsubscribe_property.user_property = NULL;

    return ESP_OK;
}

esp_err_t subscribe_to_topic(char* subscribe_topic, int qos){
    esp_err_t err = esp_mqtt5_client_set_subscribe_property(client, &subscribe_property);
    if(err != ESP_OK){
        ESP_LOGE(TAG,"Failed to set subscribe property: %s", esp_err_to_name(err));
        return err;
    }
    int err_sub = esp_mqtt_client_subscribe(client, subscribe_topic, qos);
    if(err_sub < 0){
        ESP_LOGE(TAG,"Failed to subscribe to topic");
        return ESP_FAIL;
    }
    esp_mqtt5_client_delete_user_property(subscribe_property.user_property);
    subscribe_property.user_property = NULL;

    return ESP_OK;
}

esp_err_t publish_message(char* topic, char* message, int qos, int retain)
{
    esp_err_t err = esp_mqtt5_client_set_publish_property(client, &publish_property);
    if(err != ESP_OK){
        ESP_LOGE(TAG,"Failed to set publish property: %s", esp_err_to_name(err));
        return err;
    }
    int err_publish = esp_mqtt_client_publish(client, topic, message, 0, qos, retain);
    if(err_publish < 0){
        ESP_LOGE(TAG,"Failed to send message");
        return ESP_FAIL;
    }
    esp_mqtt5_client_delete_user_property(publish_property.user_property);
    publish_property.user_property = NULL;

    return ESP_OK;
}

esp_err_t mqtt_comp_init(esp_mqtt5_user_property_item_t user_property_arr[], size_t user_array_size, char* broker_url,char* response_topic, char* last_will_topic, char* last_will_message)
{
    ESP_LOGI(TAG,"Starting MQTT...");
    mutex = xSemaphoreCreateMutex();;

    esp_mqtt5_connection_property_config_t connect_property = {
        .session_expiry_interval = 10,
        .maximum_packet_size = MQTT_BUFFER_SIZE,
        .receive_maximum = 65535,
        .topic_alias_maximum = 2,
        .request_resp_info = true,
        .request_problem_info = true,
        .will_delay_interval = 10,
        .payload_format_indicator = true,
        .message_expiry_interval = 10,
        .response_topic = response_topic,
        .correlation_data = "123456",
        .correlation_data_len = 6,
    };

    publish_property.response_topic = response_topic;

    const esp_mqtt_client_config_t mqtt5_cfg = {
        .broker = {
            .address.uri = broker_url,
        },
        .session = {
            .protocol_ver = MQTT_PROTOCOL_V_5,
            .last_will = {
                .topic = last_will_topic,
                .msg = last_will_message,
                .msg_len = MQTT_BUFFER_SIZE,
                .qos = 1,
                .retain = true,
            },
        },
    };

    client = esp_mqtt_client_init(&mqtt5_cfg);
    if(client == NULL){
        ESP_LOGE(TAG, "Failed tp init mqtt client");
        return ESP_FAIL;
    }
    
    esp_err_t err = esp_mqtt5_client_set_user_property(&connect_property.user_property, user_property_arr, user_array_size);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to set user connect property: %s",esp_err_to_name(err));
        return err;
    }
    err = esp_mqtt5_client_set_user_property(&publish_property.user_property, user_property_arr, user_array_size);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to set user publish property: %s",esp_err_to_name(err));
        return err;
    }
    err = esp_mqtt5_client_set_user_property(&subscribe_property.user_property, user_property_arr, user_array_size);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to set user subscribe property: %s",esp_err_to_name(err));
        return err;
    }
    err = esp_mqtt5_client_set_user_property(&unsubscribe_property.user_property, user_property_arr, user_array_size);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to set user unsubscribe property: %s",esp_err_to_name(err));
        return err;
    }
    err = esp_mqtt5_client_set_user_property(&connect_property.will_user_property, user_property_arr, user_array_size);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to set user will property: %s",esp_err_to_name(err));
        return err;
    }
    err = esp_mqtt5_client_set_user_property(&disconnect_property.user_property, user_property_arr, user_array_size);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to set user disconnect property: %s",esp_err_to_name(err));
        return err;
    }
    err = esp_mqtt5_client_set_connect_property(client, &connect_property);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to set connect property: %s",esp_err_to_name(err));
        return err;
    }

    /* If you call esp_mqtt5_client_set_user_property to set user properties, DO NOT forget to delete them.
     * esp_mqtt5_client_set_connect_property will malloc buffer to store the user_property and you can delete it after
     */
    esp_mqtt5_client_delete_user_property(connect_property.user_property);
    esp_mqtt5_client_delete_user_property(connect_property.will_user_property);

    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    err = esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt5_event_handler, NULL);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to register event handler: %s",esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG,"OK");
    return ESP_OK;
}
