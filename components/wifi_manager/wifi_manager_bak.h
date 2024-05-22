#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "esp_err.h"

    esp_err_t wifi_manager_init();
    esp_err_t save_wifi_config_to_nvs(const char *ssid, const char *password);
    esp_err_t clear_wifi_settings();

#ifdef __cplusplus
}
#endif

#endif
