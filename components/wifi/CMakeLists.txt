idf_component_register(SRCS "wifi.c"
                    REQUIRES esp_wifi nvs_flash littlefs cjson
                    INCLUDE_DIRS "include")

