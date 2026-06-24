idf_component_register(SRCS "https_dashboard.c"
                    REQUIRES mdns esp_https_server littlefs
                    INCLUDE_DIRS "include"
                    EMBED_TXTFILES "${CMAKE_CURRENT_SOURCE_DIR}/../../main/certificates/servercert.pem" 
                                   "${CMAKE_CURRENT_SOURCE_DIR}/../../main/certificates/prvtkey.pem")


