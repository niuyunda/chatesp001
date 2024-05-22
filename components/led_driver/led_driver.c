#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_indicator.h"
#include "led_driver.h"
#include "color.h"

#define WS2812_GPIO_NUM GPIO_NUM_48
#define WS2812_STRIPS_NUM 1

#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

#define FAST_PERIOD 200  // 快速间隔 200ms
#define SLOW_PERIOD 1000 // 慢速间隔 1000ms

// 该组件支持线程安全操作
// 您可使用全局变量共享 LED 指示灯的操作句柄 led_indicator_handle_t
// 也可以使用 led_indicator_get_handle 在其它线程通过 LED 的 IO 号获取句柄以进行操作。

static const char *TAG = "LED_INDICATOR";

led_indicator_handle_t led_handle = NULL;

// 常亮模式
// 绿色常亮
const blink_step_t green_on[] = {
    {LED_BLINK_RGB, GREEN, 0},
    {LED_BLINK_LOOP, 0, 0},
};

// 红色常亮
const blink_step_t red_on[] = {
    {LED_BLINK_RGB, RED, 0},
    {LED_BLINK_LOOP, 0, 0},
};

// 蓝色常亮
const blink_step_t blue_on[] = {
    {LED_BLINK_RGB, BLUE, 0},
    {LED_BLINK_LOOP, 0, 0},
};

// 橙色常亮
const blink_step_t orange_on[] = {
    {LED_BLINK_RGB, ORANGERED, 0},
    {LED_BLINK_LOOP, 0, 0},
};

// 呼吸模式
// 绿色慢速呼吸
const blink_step_t green_slow_breathe[] = {
    {LED_BLINK_RGB, GREEN, 0},
    {LED_BLINK_HOLD, LED_STATE_ON, 0},
    {LED_BLINK_BREATHE, LED_STATE_OFF, 1200},
    {LED_BLINK_HOLD, LED_STATE_OFF, 500},
    {LED_BLINK_BREATHE, LED_STATE_ON, 1200},
    {LED_BLINK_HOLD, LED_STATE_ON, 200},
    {LED_BLINK_LOOP, 0, 0},
};

// 绿色快速呼吸
const blink_step_t green_fast_breathe[] = {
    {LED_BLINK_RGB, GREEN, 0},
    {LED_BLINK_HOLD, LED_STATE_ON, 0},
    {LED_BLINK_BREATHE, LED_STATE_OFF, 600},
    {LED_BLINK_HOLD, LED_STATE_OFF, 200},
    {LED_BLINK_BREATHE, LED_STATE_ON, 600},
    {LED_BLINK_HOLD, LED_STATE_ON, 100},
    {LED_BLINK_LOOP, 0, 0},
};

// 橙色慢速呼吸
const blink_step_t orange_slow_breathe[] = {
    {LED_BLINK_RGB, ORANGERED, 0},
    {LED_BLINK_HOLD, LED_STATE_ON, 0},
    {LED_BLINK_BREATHE, LED_STATE_OFF, 1200},
    {LED_BLINK_HOLD, LED_STATE_OFF, 500},
    {LED_BLINK_BREATHE, LED_STATE_ON, 1200},
    {LED_BLINK_HOLD, LED_STATE_ON, 200},
    {LED_BLINK_LOOP, 0, 0},
};

// 橙色快速呼吸
const blink_step_t orange_fast_breathe[] = {
    {LED_BLINK_RGB, ORANGERED, 0},
    {LED_BLINK_HOLD, LED_STATE_ON, 0},
    {LED_BLINK_BREATHE, LED_STATE_OFF, 600},
    {LED_BLINK_HOLD, LED_STATE_OFF, 200},
    {LED_BLINK_BREATHE, LED_STATE_ON, 600},
    {LED_BLINK_HOLD, LED_STATE_ON, 100},
    {LED_BLINK_LOOP, 0, 0},
};

// 闪烁模式

// 变色模式
// 定义一个颜色渐变，让指示灯从红色渐变到蓝色，并循环执行
const blink_step_t animate_led_color_transition_loop[] = {
    {LED_BLINK_RGB, CRIMSON, 0},
    {LED_BLINK_RGB_RING, BLUE, 1000},
    {LED_BLINK_RGB_RING, LIMEGREEN, 1000},
    {LED_BLINK_RGB_RING, GOLD, 1000},
    {LED_BLINK_RGB_RING, CRIMSON, 1000},
    {LED_BLINK_LOOP, 0, 0},
};

// 将该类型对应的枚举成员，添加到闪烁类型列表 led_indicator_blink_lists 中
blink_step_t const *led_mode[] = {
    [GREEN_ON] = green_on,
    [GREEN_FAST_BREATHE] = green_fast_breathe,
    [GREEN_SLOW_BREATHE] = green_slow_breathe,
    [ORANGE_ON] = orange_on,
    [ORANGE_SLOW_BREATHE] = orange_slow_breathe,
    [RED_ON] = red_on,
    [ANIMATE_COLOR_TRANSITION] = animate_led_color_transition_loop,
    [BLINK_MAX] = NULL,
};
esp_err_t led_init(void)
{
    // LED strip configuration
    led_strip_config_t strip_config = {
        .strip_gpio_num = WS2812_GPIO_NUM,        // The GPIO that connected to the LED strip's data line
        .max_leds = WS2812_STRIPS_NUM,            // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812,            // LED strip model
        .flags.invert_out = false,                // whether to invert the output signal
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .flags.with_dma = false,               // DMA feature is available on ESP target like ESP32-S3
    };

    led_indicator_strips_config_t strips_config = {
        .led_strip_cfg = strip_config,
        .led_strip_driver = LED_STRIP_RMT,
        .led_strip_rmt_cfg = rmt_config,
    };

    const led_indicator_config_t config = {
        .mode = LED_STRIPS_MODE,
        .led_indicator_strips_config = &strips_config,
        .blink_lists = led_mode,
        .blink_list_num = BLINK_MAX,
    };
    ESP_LOGI(TAG, "Initializing LED indicator");
    led_handle = led_indicator_create(&config);
    assert(led_handle != NULL);
    return ESP_OK;
}
// 开始/停止闪烁：控制指示灯开启/停止指定闪烁类型
// 函数调用后立刻返回，内部由定时器控制闪烁流程
// 同一个指示灯可以开启多种闪烁类型，将根据闪烁类型优先级依次执行
// led_indicator_start(led_handle, ORANGE_SLOW_BREATHE);
// ESP_LOGI(TAG, "start blink: %d", ORANGE_SLOW_BREATHE);
// vTaskDelay(4000 / portTICK_PERIOD_MS);
// led_indicator_stop(led_handle, ORANGE_ON);
// ESP_LOGI(TAG, "stop blink: %d", ORANGE_ON);
// vTaskDelay(1000 / portTICK_PERIOD_MS);

// // 删除指示灯：您也可以在不需要进一步操作时，删除指示灯以释放资源
// led_indicator_delete(&led_handle);

// // 抢占操作： 您可以在任何时候直接闪烁指定的类型。
// led_indicator_preempt_start(led_handle, ORANGE_ON);

// // 停止抢占：您可以使用停止抢占函数，来取消正在抢占的闪烁模式
// led_indicator_preempt_stop(led_handle, ORANGE_ON);