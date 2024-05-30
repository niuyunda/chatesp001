idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES audio_input audio_output button_handler file_system http_handler led_driver web_server wifi_manager)
