idf_component_register(SRCS "http_handler.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_http_client cjson audio_output)
