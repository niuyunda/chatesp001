#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "led_indicator.h"
#include "led_driver.h"
#include "iot_button.h"
#include "button_handler.h"
#include "wifi_manager.h"
#include "file_system.h"
#include "audio_input.h"

static const char *TAG = "main";

void wifi_reset_button_long_press_start(void *arg, void *usr_data)
{
    ESP_LOGI(TAG, "WIFI_RESET_BUTTON_LONG_PRESS_START");
    ESP_ERROR_CHECK(clear_wifi_settings_in_nvs());
    esp_restart();
}

void main_button_press_down(void *arg, void *usr_data)
{
    ESP_LOGI(TAG, "FUNCTION_BUTTON_PRESS_DOWN");
    // led 开始进入变色模式
    led_indicator_preempt_start(led_handle, RED_TO_BLUE);
    // 开始录音
    ESP_ERROR_CHECK(record_wav(3));
}

void main_button_press_up(void *arg, void *usr_data)
{
    ESP_LOGI(TAG, "FUNCTION_BUTTON_PRESS_UP");
    // led 停止变色模式
    led_indicator_preempt_stop(led_handle, RED_TO_BLUE);

    // 停止录音
    // audio_record_stop();
    // 录音上传到服务器
    // audio_upload();
}

void app_main(void)
{
    ESP_LOGI(TAG, "启动设备，正在初始化...");

    // 初始化文件系统，包括 NVS 和 SPIFFS
    ESP_ERROR_CHECK(file_system_init());

    // 初始化 LED
    ESP_ERROR_CHECK(led_init());

    // 初始化按键
    ESP_ERROR_CHECK(button_init());
    // 注册按键回调函数
    // 长按 WIFI_RESET_BUTTON 按键，清除 WiFi 设置并重启设备
    iot_button_register_cb(wifi_reset_button, BUTTON_LONG_PRESS_START, wifi_reset_button_long_press_start, NULL);
    // 按下 FUNCTION_BUTTON 按键，开始录音
    iot_button_register_cb(main_function_button, BUTTON_PRESS_DOWN, main_button_press_down, NULL);
    // 松开 FUNCTION_BUTTON 按键，停止录音并上传到服务器
    iot_button_register_cb(main_function_button, BUTTON_PRESS_UP, main_button_press_up, NULL);

    // 初始化 WiFi
    ESP_ERROR_CHECK(wifi_manager_init());

    // 初始化音频
    // ESP_ERROR_CHECK(init_microphone());

    // // 启动录音任务
    // xTaskCreate(record_task, "record_task", 4096, NULL, 5, NULL);
}
