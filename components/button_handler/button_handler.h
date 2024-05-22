#ifndef BUTTON_H
#define BUTTON_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "iot_button.h"
    extern button_handle_t wifi_reset_button;
    extern button_handle_t main_function_button;

    esp_err_t button_init(void);

#ifdef __cplusplus
}
#endif

#endif
