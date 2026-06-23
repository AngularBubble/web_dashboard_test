idf_component_register(SRCS "https_dashboard.c"
                    REQUIRES mdns esp_https_server
                    INCLUDE_DIRS "include"
                    EMBED_TXTFILES "certificates/servercert.pem" "certificates/prvtkey.pem")
