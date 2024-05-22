#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "led_indicator.h"
#include "led_driver.h"
#include "iot_button.h"
#include "button_handler.h"
#include "wifi_manager.h"
#include "nvs_module.h"

static const char *TAG = "main";

void wifi_reset_button_long_press(void *arg, void *usr_data)
{
    ESP_LOGI(TAG, "WIFI_RESET_BUTTON_LONG_PRESS_START");
    led_indicator_preempt_start(led_handle, ORANGE_SLOW_BREATHE);
}

void app_main(void)
{
    ESP_LOGI(TAG, "启动设备，正在初始化...");

    // 初始化 NVS
    ESP_ERROR_CHECK(nvs_init());

    // LED部分
    // LED部分
    // LED部分
    ESP_ERROR_CHECK(led_init());
    led_indicator_start(led_handle, ANIMATE_COLOR_TRANSITION);
    // vTaskDelay(5000 / portTICK_PERIOD_MS);
    // led_indicator_stop(led_handle, LOOP_RED_TO_BLUE);

    // 按键部分
    // 按键部分
    // 按键部分
    ESP_ERROR_CHECK(button_init());
    iot_button_register_cb(wifi_reset_button, BUTTON_LONG_PRESS_START, wifi_reset_button_long_press, NULL);
    // iot_button_register_cb(wifi_reset_button, BUTTON_LONG_PRESS_START, wifi_reset_button_long_press, NULL);

    // 初始化WiFi
    ESP_ERROR_CHECK(wifi_manager_init());

    // // 初始化音频
    // ESP_ERROR_CHECK(audio_init());

    // // 启动WiFi配置任务
    // xTaskCreate(wifi_config_task, "wifi_config_task", 4096, NULL, 5, NULL);

    // // 启动录音任务
    // xTaskCreate(record_task, "record_task", 4096, NULL, 5, NULL);
}
