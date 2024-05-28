idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES led_driver button_handler wifi_manager nvs_module file_system audio_input)
