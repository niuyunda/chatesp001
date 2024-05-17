// #include "audio.h"
// #include "button.h"
// #include "http_server.h"
#include "wifi_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_indicator.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "启动设备，正在初始化...");

    ESP_ERROR_CHECK(led_init());

    // 示例：设置LED为绿色并快速闪烁
    // led_control(LED_COLOR_GREEN, LED_MODE_BLINK_FAST);

    // 初始化WiFi
    wifi_manager_init();

    // // 初始化HTTP服务器
    // ESP_ERROR_CHECK(http_server_init());

    // // 初始化按钮
    // ESP_ERROR_CHECK(button_init());

    // // 初始化音频
    // ESP_ERROR_CHECK(audio_init());

    // ESP_LOGI(TAG, "设备初始化完成");

    // // 启动WiFi配置任务
    // xTaskCreate(wifi_config_task, "wifi_config_task", 4096, NULL, 5, NULL);

    // // 启动录音任务
    // xTaskCreate(record_task, "record_task", 4096, NULL, 5, NULL);
}
