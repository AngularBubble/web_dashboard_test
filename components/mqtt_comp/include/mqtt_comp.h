#ifndef MQTT_COMP_H
#define MQTT_COMP_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/semphr.h"

#if __cplusplus
extern "C"
{
#endif

/** @brief Get last message received by mqtt.
 * @param buffer_destiny Buffer to receive message
 * @param buffer_size Buffer destiny size max must be 1024
 * @return TRUE if there is a new message since last check, FALSE if there is not a new message since last check.
**/
bool get_last_mqtt_message(char *buffer_destiny, size_t buffer_size);

/** @brief Subscribe to a topic.
 * @param subscribe_topic Name of the topic to be subscribed
 * @param qos Receiving qos from the topic
 * @return ESP_OK if successful, otherwise an error code.
**/
esp_err_t subscribe_to_topic(char* subscribe_topic, int qos);

/** @brief Unsubscribe from a topic.
 * @param unsubscribe_topic Name of the topic to be unsubscribed from
 * @return ESP_OK if successful, otherwise an error code.
**/
esp_err_t unsubscribe_to_topic(char* unsubscribe_topic);

/** @brief Publish message on a topic.
 * @param topic Name of the topic for publishing
 * @param message Message to be published
 * @param qos Message qos
 * @param retain Message retain flag
 * @return ESP_OK if successful, otherwise an error code.
**/
esp_err_t publish_message(char* topic, char* message, int qos, int retain);

/** @brief Publish message on a topic.
 * @param user_property_arr MQTT5 user properties array
 * @param user_array_size Size of the user_property_arr
 * @param broker_url Broker url
 * @param response_topic Publish response topic
 * @param last_will_topic Last will topic
 * @param last_will_message Last will message
 * @return ESP_OK if successful, otherwise an error code.
**/
esp_err_t mqtt_comp_init(esp_mqtt5_user_property_item_t user_property_arr[], 
                        size_t user_array_size,
                         char* broker_url,
                        char* response_topic, 
                        char* last_will_topic, 
                        char* last_will_message);

#if __cplusplus
}
#endif

#endif //SNTP_H