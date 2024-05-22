#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "nvs_module.h"
#include "led_driver.h"
#include "web_server.h"

#define WIFI_CONNECT_TIMEOUT_MS 30000 // 30秒连接超时

static const char *TAG = "WIFI_MANAGER";
static TimerHandle_t wifi_connect_timer = NULL;
static bool is_connecting = false;

// Function prototypes
static esp_err_t wifi_init(void);
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void connect_to_wifi(const char *ssid, const char *password);
static void start_ap_mode(void);
static void wifi_connect_timeout(TimerHandle_t xTimer);

// NVS load and save functions
static esp_err_t load_wifi_config_from_nvs(char *ssid, size_t ssid_size, char *password, size_t password_size);
esp_err_t save_wifi_config_to_nvs(const char *ssid, const char *password);
esp_err_t clear_wifi_settings(void);

esp_err_t wifi_manager_init()
{
    ESP_LOGI(TAG, "wifi_manager_init(): Starting WiFi Manager initialization");
    // esp_err_t ret = nvs_flash_init();
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    // {
    //     ESP_ERROR_CHECK(nvs_flash_erase());
    //     ret = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(wifi_init());

    char ssid[32] = {0};
    char password[64] = {0};
    if (load_wifi_config_from_nvs(ssid, sizeof(ssid), password, sizeof(password)) == ESP_OK)
    {
        ESP_LOGI(TAG, "WiFi settings found, connecting to WiFi");
        connect_to_wifi(ssid, password);
    }
    else
    {
        ESP_LOGI(TAG, "WiFi settings not found, switching to AP mode for configuration");
        start_ap_mode();
    }
    return ESP_OK;
}

static esp_err_t wifi_init()
{
    ESP_LOGI(TAG, "wifi_init(): Initializing WiFi");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    return ESP_OK;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    static bool is_connected = false;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "WiFi connecting...");
        led_indicator_preempt_start(led_handle, GREEN_SLOW_BREATHE);
        wifi_connect_timer = xTimerCreate("wifi_connect_timer", pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS), pdFALSE, NULL, wifi_connect_timeout);
        xTimerStart(wifi_connect_timer, 0);
        is_connected = false;
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (is_connected)
        {
            ESP_LOGI(TAG, "WiFi disconnected, retrying...");
            esp_wifi_connect();
            // set_led_color_and_mode(LED_COLOR_ORANGE, LED_MODE_ON);
        }
        else
        {
            ESP_LOGI(TAG, "WiFi connection failed, retrying...");
            esp_wifi_connect();
            // set_led_color_and_mode(LED_COLOR_GREEN, LED_MODE_BLINK_SLOW);
        }
        if (wifi_connect_timer)
        {
            xTimerStop(wifi_connect_timer, 0);
            xTimerStart(wifi_connect_timer, 0);
        }
        is_connected = false;
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG, "WiFi connected!");
        // set_led_color_and_mode(LED_COLOR_GREEN, LED_MODE_ON);
        if (wifi_connect_timer)
        {
            xTimerStop(wifi_connect_timer, 0);
            xTimerDelete(wifi_connect_timer, 0);
            wifi_connect_timer = NULL;
        }
        is_connected = true;
        ESP_LOGI(TAG, "WiFi connected finish!!!!!!");
    }
}

static void wifi_connect_timeout(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "WiFi connection timed out, connection failed");
    esp_wifi_stop();
    // set_led_color_and_mode(LED_COLOR_ORANGE, LED_MODE_ON);
}

static void connect_to_wifi(const char *ssid, const char *password)
{
    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);

    ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid);
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();
    is_connecting = true;
}

static void start_ap_mode()
{
    ESP_LOGI(TAG, "Starting AP mode");
    // 进入 ap 模式，led 指示灯为橙色慢呼吸模式
    led_indicator_preempt_start(led_handle, ORANGE_SLOW_BREATHE);

    esp_netif_create_default_wifi_ap();

    wifi_config_t ap_config = {
        .ap = {
            .ssid = "ESP32_WIFI_SETUP",
            .ssid_len = strlen("ESP32_WIFI_SETUP"),
            .max_connection = 2,
            .authmode = WIFI_AUTH_OPEN},
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP mode enabled, connect to WiFi: ESP32_WIFI_SETUP");
    start_web_server();
    ESP_LOGI(TAG, "Web server started");
}

static esp_err_t load_wifi_config_from_nvs(char *ssid, size_t ssid_size, char *password, size_t password_size)
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

esp_err_t save_wifi_config_to_nvs(const char *ssid, const char *password)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("wifi_config", NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK)
        return ret;

    ret = nvs_set_str(nvs_handle, "ssid", ssid);
    if (ret == ESP_OK)
    {
        ret = nvs_set_str(nvs_handle, "password", password);
    }

    if (ret == ESP_OK)
    {
        ret = nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);
    return ret;
}

esp_err_t clear_wifi_settings_in_nvs()
{
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("wifi_config", NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_erase_all(nvs_handle);
    if (ret == ESP_OK)
    {
        ret = nvs_commit(nvs_handle);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to clear WiFi settings: %s", esp_err_to_name(ret));
    }

    nvs_close(nvs_handle);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "WiFi settings cleared");
    }

    return ret;
}
