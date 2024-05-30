// #include <stdio.h>
// #include <string.h>
// #include <math.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "format_wav.h"

#define INMP441_SLK GPIO_NUM_15
#define INMP441_WS GPIO_NUM_16
#define INMP441_SD GPIO_NUM_17

#define BIT_SAMPLE I2S_DATA_BIT_WIDTH_16BIT
#define SAMPLE_RATE 16000
#define NUM_CHANNELS (1) // For mono recording only!

static const char *TAG = "AUDIO_INPUT";

#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
#define SAMPLE_SIZE (BIT_SAMPLE * 1024)
#define BYTE_RATE (SAMPLE_RATE * (BIT_SAMPLE / 8)) * NUM_CHANNELS
#define RECORD_FILE_PATH "/spiffs/record.wav"

i2s_chan_handle_t rx_handle = NULL;

static int16_t i2s_readraw_buff[SAMPLE_SIZE];
size_t bytes_read;
const int WAVE_HEADER_SIZE = 44;

esp_err_t init_microphone(void)
{
    i2s_chan_config_t rx_handle_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&rx_handle_cfg, NULL, &rx_handle));

    i2s_std_config_t inmp441_i2s_config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = INMP441_SLK,
            .ws = INMP441_WS,
            .dout = I2S_GPIO_UNUSED,
            .din = INMP441_SD,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    inmp441_i2s_config.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &inmp441_i2s_config));
    return ESP_OK;
}

esp_err_t record_wav(uint32_t rec_time)
{
    ESP_LOGI(TAG, "record_wav() Start Record");

    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));

    // Use POSIX and C standard library functions to work with files.
    int flash_wr_size = 0;
    ESP_LOGI(TAG, "record_wav() Opening file");

    uint32_t flash_rec_time = BYTE_RATE * rec_time;
    // WAV_HEADER_PCM_DEFAULT(wav_sample_size, wav_sample_bits, wav_sample_rate, wav_channel_num)
    const wav_header_t wav_header =
        WAV_HEADER_PCM_DEFAULT(flash_rec_time, BIT_SAMPLE, SAMPLE_RATE, NUM_CHANNELS);

    // 在创建新文件前，检查该文件是否已经存在
    struct stat st;
    if (stat(RECORD_FILE_PATH, &st) == 0)
    {
        ESP_LOGI(TAG, "record_wav()  %s 存在文件，正在删除文件", RECORD_FILE_PATH);
        // 如果存在就删除文件
        unlink(RECORD_FILE_PATH);
    }

    // 创建新的 WAV 文件
    FILE *f = fopen(RECORD_FILE_PATH, "a");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "record_wav() 打开文件失败，无法写入文件");
        return ESP_OK;
    }

    // Write the header to the WAV file
    fwrite(&wav_header, sizeof(wav_header), 1, f);

    // 开始录音
    while (flash_wr_size < flash_rec_time)
    {
        // 为原始样本分配内存
        // char *i2s_raw_buffer = heap_caps_calloc(1, SAMPLE_SIZE, MALLOC_CAP_SPIRAM);

        // 从麦克风读取原始样本数据
        if (i2s_channel_read(rx_handle, (char *)i2s_readraw_buff, SAMPLE_SIZE, &bytes_read, 1000) == ESP_OK)
        {
            // printf("[0] %d [1] %d [2] %d [3]%d ...\n", i2s_readraw_buff[0], i2s_readraw_buff[1], i2s_readraw_buff[2], i2s_readraw_buff[3]);
            // Write the samples to the WAV file
            fwrite(i2s_readraw_buff, bytes_read, 1, f);
            flash_wr_size += bytes_read;
        }
        else
        {
            printf("record_wav() 读取原始样本数据失败\n");
        }
    }

    ESP_LOGI(TAG, "record_wav() 录制完成！");
    fclose(f);
    ESP_LOGI(TAG, "record_wav() 文件写入成功！");

    /* 删除通道之前必须先禁用通道 */
    i2s_channel_disable(rx_handle);
    /* 如果不再需要句柄，删除该句柄以释放通道资源 */
    // i2s_del_channel(rx_handle);

    return ESP_OK;
}
