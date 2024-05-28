#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "led_indicator.h"
#include "color.h"

    extern led_indicator_handle_t led_handle;

    // 定义闪烁类型之后，需要在 led_indicator_blink_type_t 添加该类型对应的枚举成员
    typedef enum
    {
        GREEN_ON,
        GREEN_FAST_BREATHE,
        GREEN_SLOW_BREATHE,
        ORANGE_ON,
        ORANGE_SLOW_BREATHE,
        RED_ON,
        RED_TO_BLUE,
        ANIMATE_COLOR_TRANSITION,
        BLINK_MAX, /**< INVALIED type */
    } led_indicator_blink_type_t;

    esp_err_t led_init(void);
#ifdef __cplusplus
}
#endif

#endif
