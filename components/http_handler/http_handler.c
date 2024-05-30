// #include "app_http_asr.h"
// #include "app_spiffs.h"
// #include "hal_i2s.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_err.h"
// #include "json_parser.h"
#include "http_handler.h"
#include "cJSON.h"

// #define BOUNDARY "--------WebKitFormBoundary7MA4YWxkTrZu0gW"
// #define FILE_FIELD_NAME "file"
// #define FILENAME "record.wav"

static const char *TAG = "HTTP_HANDLER";

// 我的服务器
// char *access_token = "";
char *url = "http://192.168.0.10:8000/llm/";

char chat_result[1024] = {};

esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA)
    {
        // 使用ESP_LOGI宏打印接收到的数据内容到日志，方便调试。
        ESP_LOGI(TAG, "%.*s", evt->data_len, (char *)evt->data);

        // 使用 cJSON 解析接收到的数据
        // 解析 JSON 字符串
        cJSON *json = cJSON_Parse((char *)evt->data);

        // 获取并打印 "result" 的值
        const cJSON *result = cJSON_GetObjectItemCaseSensitive(json, "result");
        if (cJSON_IsString(result) && (result->valuestring != NULL))
        {
            strcpy(chat_result, result->valuestring);
        }

        // 清理 cJSON 对象
        cJSON_Delete(json);
    }
    // 当 evt -> event_id 等于 HTTP_EVENT_ON_FINISH，表示 HTTP 请求完成。
    else if (evt->event_id == HTTP_EVENT_ON_FINISH)
    {
        ESP_LOGI(TAG, "http_event_handler() HTTP_EVENT_ON_FINISH");
        // // 检查全局指针xLlmQuestion是否非空，确保有一个有效的消息队列可以发送数据
        // if (xLlmQuestion != NULL)
        // {
        //     // 如果队列存在，通过 xQueueSend 函数将解析得到的语音识别结果 chat_result 发送到队列中
        //     // 并设置超时时间为100个ticks（具体时间取决于系统tick频率）。
        //     xQueueSend(xLlmQuestion, chat_result, 100);

        //     // 通过 memset 函数将 chat_result 数组清零，以便下一次使用。
        //     memset(chat_result, 0, sizeof(chat_result));
        // }
    }

    return ESP_OK;
}

esp_err_t audio_upload(int time)
{
    // 调用hal_i2s_record函数，以指定的时间（time参数）
    // 录制音频数据到 SPIFFS 文件系统中的 /spiffs/record.wav 文件。
    // hal_i2s_record("/spiffs/record.wav", time);

    // 打开录制的.wav文件
    // 如果文件打开失败，会通过ESP_LOGI打印一条错误信息
    FILE *wav_file = fopen("/spiffs/record.wav", "rb");
    if (wav_file == NULL)
    {
        ESP_LOGE(TAG, "Failed to open audio file");
        return ESP_FAIL;
    }

    // 使用SEEK_END作为定位方式，将文件指针 wav_file 移动到文件末尾
    fseek(wav_file, 0, SEEK_END);

    // 获取当前文件的大小，并将其赋值给 wav_file_len 变量
    // 使用 ftell 函数获取文件当前位置（即文件末尾位置）
    size_t wav_file_len = ftell(wav_file);

    // 使用SEEK_SET作为定位方式，将文件指针wav_file重新定位到文件开头
    fseek(wav_file, 0, SEEK_SET);

    // 使用ESP_LOGI宏函数打印日志信息，输出WAV文件的大小
    ESP_LOGI(TAG, "WAV File size:%zu", wav_file_len);

    // 使用 heap_caps_malloc 动态分配一块内存 wav_file_len+1，并赋值给 wav_raw_buffer
    // 该内存用于存储WAV文件数据，以供后续处理
    char *wav_raw_buffer = heap_caps_malloc(wav_file_len + 1, MALLOC_CAP_SPIRAM);

    // 如果分配失败，则打印错误信息并直接返回。
    if (wav_raw_buffer == NULL)
    {
        ESP_LOGI(TAG, "Malloc wav raw buffer fail");
        return ESP_FAIL;
    }

    // size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
    // ptr -- 这是指向带有最小尺寸 size*nmemb 字节的内存块的指针。
    // size -- 这是要读取的每个元素的大小，以字节为单位。
    // nmemb -- 这是元素的个数，每个元素的大小为 size 字节。
    // stream -- 这是指向 FILE 对象的指针，该 FILE 对象指定了一个输入流。
    fread(wav_raw_buffer, wav_file_len, 1, wav_file);

    // 确保在函数结束时释放 wav_raw_buffer，防止内存泄漏。
    fclose(wav_file);

    // 初始化 esp_http_client_config_t 结构体
    esp_http_client_config_t config = {
        // url
        .url = url,
        // HTTP 方法为 POST
        .method = HTTP_METHOD_POST,
        // 事件处理函数 http_event_handler
        .event_handler = http_event_handler,
        // 设置缓冲区大小
        .buffer_size = 4 * 1024,
    };

    // 分配一块动态内存，用于存储拼接后的字符串
    // MALLOC_CAP_SPIRAM 是ESP32微控制器上的一个标志，用于指定内存分配应该在SPI RAM（即外部串行RAM）中进行
    // ESP32芯片内部有集成的RAM，但有时为了增加存储容量，可以外接 SPI RAM。这种外置 RAM 通常速度较慢，但可以显著扩展可用内存。
    // 当调用 heap_caps_malloc() 函数并传递 MALLOC_CAP_SPIRAM 参数时
    // 系统会确保分配的内存位于 SPI RAM 中，这有助于在处理大数据或需要更多内存的应用场景下避免内部RAM不足的问题。
    // char *url = heap_caps_malloc(strlen(url_formate) + strlen(access_token) + 1, MALLOC_CAP_SPIRAM);

    // 将 url_formate 格式字符串中的 access_token 插入到url字符串中，根据格式化字符串生成新的字符串
    // sprintf 函数在C语言中是一个格式化输出函数
    // 它的主要用途是将格式化的数据写入到一个字符数组（字符串）中
    // sprintf(url, url_formate, access_token);
    // config.url = url;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // 设置请求头
    // esp_http_client_set_header(client, "Content-Type", "multipart/form-data; boundary=" BOUNDARY);
    // esp_http_client_set_header(client, "Content-Type", "audio/wav");
    esp_http_client_set_header(client, "Accept", "application/json");

    // 这个boundary可以自定义，但必须确保在HTTP体中是唯一的

    // // 构建请求体的开始部分
    // char *post_data_start;
    // asprintf(&post_data_start,
    //          "--%s\r\n"
    //          "Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"
    //          "Content-Type: audio/wav\r\n\r\n",
    //          BOUNDARY, FILE_FIELD_NAME, FILENAME);

    // // 构建请求体的结束部分
    // char *post_data_end;
    // asprintf(&post_data_end, "\r\n--%s--\r\n", BOUNDARY);

    // // 发送请求体的开始部分
    // esp_http_client_write(client, post_data_start, strlen(post_data_start));

    // // 发送文件数据，wav_raw_buffer 是文件的二进制数据，wav_file_len 是数据长度
    // esp_http_client_write(client, wav_raw_buffer, wav_file_len);

    // // 发送请求体的结束部分
    // esp_http_client_write(client, post_data_end, strlen(post_data_end));

    esp_http_client_set_post_field(client, wav_raw_buffer, wav_file_len);

    // 执行HTTP请求
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", esp_http_client_get_status_code(client), (int)esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGI(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    free(wav_raw_buffer);
    // free(post_data_start);
    // free(post_data_end);
    esp_http_client_cleanup(client);

    return err;
}