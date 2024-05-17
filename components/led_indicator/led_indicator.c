#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "led_strip.h"
#include "led_indicator.h"

static const char *TAG = "LED_INDICATOR";

#define GPIO 48

static uint8_t s_led_state = 0;
static TaskHandle_t xHandleLedControl = NULL;
static led_strip_handle_t led_strip = NULL;
static QueueHandle_t xQueueLedControl = NULL;

typedef struct
{
    led_color_t color;
    led_mode_t mode;
} led_control_params_t;

// LED控制任务声明
void led_control_task(void *pvParameters);

// LED初始化函数
esp_err_t led_init(void)
{
    ESP_LOGI(TAG, "Initializing RGB LED");

    led_strip_config_t strip_config = {
        .strip_gpio_num = GPIO,
        .max_leds = 1,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create LED strip object: %s", esp_err_to_name(ret));
        return ret;
    }

    led_strip_clear(led_strip);

    // 创建控制队列
    xQueueLedControl = xQueueCreate(10, sizeof(led_control_params_t));
    if (xQueueLedControl == NULL)
    {
        ESP_LOGE(TAG, "Failed to create LED control queue");
        return ESP_FAIL;
    }

    // 创建LED控制任务
    BaseType_t task_created = xTaskCreate(led_control_task, "LED Control Task", 2048, NULL, 5, &xHandleLedControl);
    if (task_created != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create LED control task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

// LED控制任务
void led_control_task(void *pvParameters)
{
    led_control_params_t params;
    uint32_t delay_time = 0;
    uint8_t red = 0, green = 0, blue = 0;

    while (1)
    {
        if (xQueueReceive(xQueueLedControl, &params, portMAX_DELAY))
        {
            // 根据颜色设置RGB值
            switch (params.color)
            {
            case LED_COLOR_RED:
                red = 255;
                green = 0;
                blue = 0;
                break;
            case LED_COLOR_ORANGE:
                red = 255;
                green = 97;
                blue = 0;
                break;
            case LED_COLOR_GREEN:
                red = 0;
                green = 255;
                blue = 0;
                break;
            }

            switch (params.mode)
            {
            case LED_MODE_OFF:
                led_strip_clear(led_strip);
                led_strip_refresh(led_strip);
                break;
            case LED_MODE_ON:
                led_strip_set_pixel(led_strip, 0, red, green, blue);
                led_strip_refresh(led_strip);
                break;
            case LED_MODE_BLINK_FAST:
                delay_time = 100; // 快速闪烁100ms
                break;
            case LED_MODE_BLINK_SLOW:
                delay_time = 500; // 慢速闪烁500ms
                break;
            }

            if (params.mode == LED_MODE_BLINK_FAST || params.mode == LED_MODE_BLINK_SLOW)
            {
                while (1)
                {
                    s_led_state = !s_led_state;
                    if (s_led_state)
                    {
                        led_strip_set_pixel(led_strip, 0, red, green, blue);
                    }
                    else
                    {
                        led_strip_clear(led_strip);
                    }
                    led_strip_refresh(led_strip);
                    vTaskDelay(delay_time / portTICK_PERIOD_MS);
                }
            }
        }
    }
}

// LED控制函数
void led_control(led_color_t color, led_mode_t mode)
{
    if (xHandleLedControl != NULL && xQueueLedControl != NULL)
    {
        led_control_params_t params = {.color = color, .mode = mode};
        if (xQueueSend(xQueueLedControl, &params, portMAX_DELAY) != pdPASS)
        {
            ESP_LOGE(TAG, "Failed to send to LED control queue");
        }
    }
    else
    {
        ESP_LOGE(TAG, "LED control task or queue is not created");
    }
}
