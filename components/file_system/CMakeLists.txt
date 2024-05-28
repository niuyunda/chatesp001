idf_component_register(SRCS "file_system.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash spiffs)
