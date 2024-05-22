#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "iot_button.h"
#include "button_handler.h"

#define BUTTON_LONG_PRESS_TIME_MS 5000 // 5秒长按时间
#define WIFI_RESET_BUTTON_GPIO GPIO_NUM_0
#define MAIN_FUNCTION_BUTTON_GPIO GPIO_NUM_48

static const char *TAG = "BUTTON";

button_handle_t wifi_reset_button = NULL;
button_handle_t main_function_button = NULL;

// static button_handle_t gpio_btn_init(int gpio_num);

esp_err_t button_init(void)
{
    // 初始化wifi重置按钮
    button_config_t wifi_reset_button_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
        .gpio_button_config = {
            .gpio_num = WIFI_RESET_BUTTON_GPIO,
            .active_level = 0,
        },
    };
    wifi_reset_button = iot_button_create(&wifi_reset_button_cfg);
    if (NULL == wifi_reset_button)
    {
        ESP_LOGE(TAG, "Wifi Reset Button create failed");
    }
    ESP_LOGI(TAG, "Wifi Reset Button created successfully, GPIO number: %d", WIFI_RESET_BUTTON_GPIO);

    // 初始化主功能按钮
    button_config_t main_function_button_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
        .gpio_button_config = {
            .gpio_num = WIFI_RESET_BUTTON_GPIO,
            .active_level = 0,
        },
    };
    main_function_button = iot_button_create(&main_function_button_cfg);
    if (NULL == main_function_button)
    {
        ESP_LOGE(TAG, "Main Function Button Reset Button create failed");
    }
    ESP_LOGI(TAG, "Main Function Button created successfully, GPIO number: %d", MAIN_FUNCTION_BUTTON_GPIO);

    // wifi_reset_button = gpio_btn_init(WIFI_RESET_BUTTON_GPIO);
    // assert(wifi_reset_button != NULL);

    // 初始化主功能按钮
    // main_function_button = gpio_btn_init(MAIN_FUNCTION_BUTTON_GPIO);
    // assert(main_function_button != NULL);
    return ESP_OK;
};

// static button_handle_t gpio_btn_init(int gpio_num)
// {
//     // 初始化按钮
//     button_config_t gpio_btn_cfg = {
//         .type = BUTTON_TYPE_GPIO,
//         .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
//         .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
//         .gpio_button_config = {
//             .gpio_num = gpio_num,
//             .active_level = 0,
//         },
//     };
//     button_handle_t gpio_btn = iot_button_create(&gpio_btn_cfg);
//     if (NULL == gpio_btn)
//     {
//         ESP_LOGE(TAG, "Button create failed");
//     }
//     ESP_LOGI(TAG, "Button created successfully, GPIO number: %d", gpio_num);
//     return gpio_btn;
// }

// // 查询按键事件
// button_event_t event;
// event = iot_button_get_event(button_handle);

// // 动态修改按键默认值
// iot_button_set_param(btn, BUTTON_LONG_PRESS_TIME_MS, 5000);