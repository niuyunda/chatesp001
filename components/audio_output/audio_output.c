#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "driver/gpio.h"

#define MAX98357A_BCLK GPIO_NUM_9
#define MAX98357A_LRC GPIO_NUM_10
#define MAX98357A_DIN GPIO_NUM_11
#define MAX98357A_SD GPIO_NUM_12

static const char *TAG = "AUDIO_OUTPUT";

i2s_chan_handle_t tx_handle = NULL;

esp_err_t init_speaker(void)
{
    ESP_LOGI(TAG, "init_speaker() Start...");

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

    i2s_tdm_config_t tdm_config = {
        .clk_cfg = I2S_TDM_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_TDM_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO, I2S_TDM_SLOT0),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = MAX98357A_BCLK,
            .dout = MAX98357A_DIN,
            .ws = MAX98357A_LRC,
            .invert_flags = {
                .bclk_inv = false,
                .mclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    tdm_config.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_512;
    i2s_channel_init_tdm_mode(tx_handle, &tdm_config);
    i2s_channel_enable((tx_handle));

    // 将 MAX98357A_SD 设置为 HIGH，使 MAX98357A 进入正常工作状态
    gpio_set_direction(MAX98357A_SD, GPIO_MODE_OUTPUT);
    gpio_set_level(MAX98357A_SD, 1);

    return ESP_OK;
}
