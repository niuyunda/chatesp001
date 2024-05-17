#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

// 枚举类型定义
typedef enum
{
    LED_COLOR_RED,
    LED_COLOR_ORANGE,
    LED_COLOR_GREEN
} led_color_t;

typedef enum
{
    LED_MODE_OFF,
    LED_MODE_ON,
    LED_MODE_BLINK_FAST,
    LED_MODE_BLINK_SLOW
} led_mode_t;

// 函数声明
esp_err_t led_init(void);
void led_control(led_color_t color, led_mode_t mode);

#endif // LED_INDICATOR_H
