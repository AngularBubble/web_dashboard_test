#include <stdio.h>
#include "sntp.h"

static const char *TAG = "SNTP";

static void sntp_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base; (void)id;
    const esp_netif_sntp_time_sync_t *evt = (const esp_netif_sntp_time_sync_t *)data;
    if (evt) {
        char ts[64];
        time_t t = evt->tv.tv_sec;
        struct tm tm_utc;
        gmtime_r(&t, &tm_utc);
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm_utc);
        ESP_LOGI(TAG, "SNTP event: time synced (UTC): %s.%06ld", ts, (long)evt->tv.tv_usec);
    } else {
        ESP_LOGI(TAG, "SNTP event: time synced (no timeval provided)");
    }
}

static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

esp_err_t sntp_comp_init(char* server_name)
{
    ESP_LOGI(TAG,"Starting sntp...");
    esp_err_t err = esp_event_handler_register(NETIF_SNTP_EVENT, NETIF_SNTP_TIME_SYNC, &sntp_event_handler, NULL);
    if(err != ESP_OK){
        ESP_LOGI(TAG,"Failed to register NETIF SNTP EVENT handler: %s", esp_err_to_name(err));
        return err;
    }
/*
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);
    config.start = false;                       // start SNTP service explicitly (after connecting)
    config.server_from_dhcp = true;             // accept NTP offers from DHCP server, if any (need to enable *before* connecting)
    config.renew_servers_after_new_IP = true;   // let esp-netif update configured SNTP server(s) after receiving DHCP lease
    config.index_of_first_server = 1;           // updates from server num 1, leaving server 0 (from DHCP) intact
    // configure the event on which we renew servers
    config.ip_event_to_renew = IP_EVENT_STA_GOT_IP;
    config.sync_cb = time_sync_notification_cb; // only if we need the notification function
    esp_netif_sntp_init(&config);
*/
    return ESP_OK;
}
