/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/*  WiFi softAP & station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_net_stack.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#if IP_NAPT
#include "lwip/lwip_napt.h"
#endif
#include "lwip/err.h"
#include "lwip/sys.h"
#include "led_driver.h"
#include "web_server.h"

/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_ESP_WIFI_STA_SSID "mywifissid"
*/

/* STA Configuration */
// #define WIFI_STA_SSID "ESP32_WIFI_SETUP"
#define ESP_MAXIMUM_RETRY 3

// #if CONFIG_ESP_WIFI_AUTH_OPEN
// #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
// #elif CONFIG_ESP_WIFI_AUTH_WEP
// #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
// #elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
// #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
// #elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
// #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
// #elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
// #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
// #elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
// #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
// #elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
// #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
// #elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
// #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
// #endif

// /* AP Configuration */
#define WIFI_STA_SSID "ESP32_WIFI_SETUP"
#define WIFI_STA_CHANNEL 6
#define MAX_STA_CONN 1

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG_AP = "WIFI_MANAGER_AP";
static const char *TAG_STA = "WIFI_MANAGER_STA";
// static const char *TAG = "WIFI_MANAGER";

static int s_retry_num = 0;

/* FreeRTOS event group to signal when we are connected/disconnected */
static EventGroupHandle_t s_wifi_event_group;
static esp_netif_t *esp_netif_ap;
static esp_netif_t *esp_netif_sta;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG_AP, "wifi_event_handler() 客户端 " MACSTR " 加入, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG_AP, "wifi_event_handler() 客户端 " MACSTR " 离开, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_ERROR_CHECK(esp_wifi_connect());
        ESP_LOGI(TAG_STA, "wifi_event_handler() STA 模式启动，开始连接 WiFi");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGE(TAG_STA, "wifi_event_handler() WiFi 连接错误，可能是因为 ssid 或密码错误，请重新配置");
        // led 指示灯为橙色常亮模式
        led_indicator_preempt_start(led_handle, ORANGE_ON);
        // 关闭 WiFi
        // ESP_ERROR_CHECK(esp_wifi_stop());
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_STA, "wifi_event_handler() 获得 IP 地址:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* Initialize soft AP */
esp_netif_t *wifi_init_softap()
{
    // esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();
    esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = WIFI_STA_SSID,
            .ssid_len = strlen(WIFI_STA_SSID),
            .channel = WIFI_STA_CHANNEL,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_OPEN,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    // if (strlen(password) == 0)
    // {
    //     wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    // }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    ESP_LOGI(TAG_AP, "wifi_init_softap() AP 模式开启，请连接 WiFi：%s",
             WIFI_STA_SSID);
    return esp_netif_ap;
}

/* Initialize wifi station */
esp_netif_t *wifi_init_sta(char *ssid, char *password)
{
    // ssid = "myrouter";
    // password = "AAA128128";
    // 输出 正在连接 WiFi
    ESP_LOGI(TAG_STA, "wifi_init_sta() 准备连接 WiFi: %s，密码：%s", ssid, password);

    // 进入 sta 模式，led 指示灯为绿色快呼吸模式
    led_indicator_preempt_start(led_handle, GREEN_FAST_BREATHE);

    // esp_netif_t *esp_netif_sta = esp_netif_create_default_wifi_sta();
    esp_netif_t *esp_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_sta_config = {
        .sta = {
            // .ssid = ssid,
            // .password = password,
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .failure_retry_cnt = ESP_MAXIMUM_RETRY,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            // .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    strncpy((char *)wifi_sta_config.sta.ssid, ssid, sizeof(wifi_sta_config.sta.ssid) - 1);
    strncpy((char *)wifi_sta_config.sta.password, password, sizeof(wifi_sta_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));

    ESP_LOGI(TAG_STA, "wifi_init_sta() WiFi STA 初始化完成");

    return esp_netif_sta;
}

static esp_err_t load_wifi_config(char *ssid, size_t ssid_size, char *password, size_t password_size)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("wifi_config", NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK)
        return ret;

    ret = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_size);
    if (ret == ESP_OK)
    {
        ret = nvs_get_str(nvs_handle, "password", password, &password_size);
    }

    nvs_close(nvs_handle);
    return ret;
}

esp_err_t wifi_manager_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize event group */
    s_wifi_event_group = xEventGroupCreate();

    /* Register Event handler */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    /*Initialize WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    char ssid[32] = {0};
    char password[64] = {0};
    if (load_wifi_config(ssid, sizeof(ssid), password, sizeof(password)) == ESP_OK)
    {
        ESP_LOGI(TAG_STA, "NVS 发现 WiFi 配置，启动 STA 模式");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

        /* Initialize STA */
        esp_netif_ap = wifi_init_sta(ssid, password);
    }
    else
    {
        ESP_LOGI(TAG_AP, "NVS 未发现 WiFi 配置，启动 AP 模式");
        // 进入 ap 模式，led 指示灯为橙色慢呼吸模式
        led_indicator_preempt_start(led_handle, ORANGE_SLOW_BREATHE);
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

        /* Initialize AP */
        esp_netif_sta = wifi_init_softap();
        ESP_ERROR_CHECK(start_web_server());
    }

    // /* Initialize AP */
    // ESP_LOGI(TAG_AP, "ESP_WIFI_MODE_AP");
    // esp_netif_t *esp_netif_ap = wifi_init_softap();

    // /* Initialize STA */
    // ESP_LOGI(TAG_STA, "ESP_WIFI_MODE_STA");
    // esp_netif_t *esp_netif_sta = wifi_init_sta();

    /* Start WiFi */
    ESP_ERROR_CHECK(esp_wifi_start());

    /*
     * Wait until either the connection is established (WIFI_CONNECTED_BIT) or
     * connection failed for the maximum number of re-tries (WIFI_FAIL_BIT).
     * The bits are set by event_handler() (see above)
     */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    /* xEventGroupWaitBits() returns the bits before the call returned,
     * hence we can test which event actually happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG_STA, "xEventGroupWaitBits() 连接 WiFi 网络 SSID:%s password:%s",
                 ssid, password);
        // 成功连接到 WiFi，led 指示灯为绿色常亮模式
        led_indicator_preempt_start(led_handle, GREEN_ON);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG_STA, "xEventGroupWaitBits() 连接 WiFi 失败 SSID:%s, password:%s",
                 ssid, password);
        // 无法连接到 WiFi，led 指示灯为橙色常亮模式
        led_indicator_preempt_start(led_handle, ORANGE_ON);
    }
    else
    {
        // 未知事件，led 指示灯为红色常亮模式
        led_indicator_preempt_start(led_handle, RED_ON);
        ESP_LOGE(TAG_STA, "xEventGroupWaitBits() 未知事件");
        return ESP_FAIL;
    }

    // /* Set sta as the default interface */
    // esp_netif_set_default_netif(esp_netif_ap);

    // /* Enable napt on the AP netif */
    // if (esp_netif_napt_enable(esp_netif_ap) != ESP_OK)
    // {
    //     ESP_LOGE(TAG_STA, "NAPT not enabled on the netif: %p", esp_netif_ap);
    // }

    return ESP_OK;
}

// esp_err_t check_wifi_config(char *ssid, char *password)
// {

//     return ESP_OK;
// }