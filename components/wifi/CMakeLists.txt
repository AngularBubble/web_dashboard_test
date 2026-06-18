idf_component_register(SRCS "wifi.c"
                    REQUIRES esp_wifi nvs_flash littlefs
                    INCLUDE_DIRS "include")

littlefs_create_partition_image(web_data ../wifi/data FLASH_IN_PROJECT)