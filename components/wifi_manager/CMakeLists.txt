idf_component_register(SRCS "wifi_manager.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash esp_wifi web_server led_indicator)
