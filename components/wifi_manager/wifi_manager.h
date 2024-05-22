#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "esp_err.h"
    esp_err_t check_wifi_config(char *ssid, char *password);
    esp_err_t wifi_manager_init();
    esp_err_t wifi_init_sta(char *ssid, char *password);

#ifdef __cplusplus
}
#endif

#endif
