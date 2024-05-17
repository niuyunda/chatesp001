#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "string.h"
#include "led_indicator.h"
#include "web_server.h" // Include the web server header

static const char *TAG = "WI-FI_MANAGER";

// Function prototypes
static esp_err_t wifi_init(void);
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void connect_to_wifi(const char *ssid, const char *password);
static void start_ap_mode(void);

// NVS load and save functions
static esp_err_t load_wifi_config(char *ssid, size_t ssid_size, char *password, size_t password_size);
esp_err_t save_wifi_config(const char *ssid, const char *password);

// TODO: 保存wifi配置后，提示保存成功（对话框或者通知），地址要回到主页，不要停留在提交的地址
// TODO: 设备一直在尝试连接，应该有一个超时机制，超时后提示连接失败。并且通过按主板上的按键，可以重新配置wifi
// TODO: 指示灯不正确，连接成功后，指示灯应该常亮，而不是闪烁。

//  指示灯几种状态：
//  绿色常亮（连接成功）
//  绿色快闪（连接中）
//  橙色常亮（连接失败，可能是信号不好，用户名密码错）
//  橙色慢闪（热点模式，等待配网）

esp_err_t wifi_manager_init()
{
    ESP_LOGI(TAG, "wifi_manager_init()：开始初始化WiFi管理器");
    // Initialize NVS
    ESP_LOGI(TAG, "wifi_manager_init()：开始初始化NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize WiFi
    ESP_ERROR_CHECK(wifi_init());

    char ssid[32], password[64]; // Adjust sizes as needed
    if (load_wifi_config(ssid, sizeof(ssid), password, sizeof(password)) == ESP_OK)
    {
        connect_to_wifi(ssid, password);
    }
    else
    {
        ESP_LOGI(TAG, "未找到WiFi设置，转为WiFi热点模式，请连接设备WiFi后进行配置");
        // Set LED to error state or AP mode indication
        led_control(LED_COLOR_ORANGE, LED_MODE_BLINK_SLOW);
        start_ap_mode();
    }
    return ESP_OK;
}

static esp_err_t wifi_init()
{
    ESP_LOGI(TAG, "wifi_init()：开始初始化WiFi");
    // 1. 初始化网络接口和事件循环系统
    // 调用 esp_netif_init() 函数初始化网络接口层，这是ESP-IDF框架中用于管理网络接口的基础组件。
    ESP_ERROR_CHECK(esp_netif_init());

    // 调用 esp_event_loop_create_default() 函数创建默认的事件循环，这是ESP-IDF框架中用于处理事件和消息的基础组件。
    // 事件循环是处理系统异步事件（如Wi-Fi连接状态改变、获取IP地址等）的核心机制。
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 2. 创建默认Wi-Fi Station接口
    // esp_netif_create_default_wifi_sta() 函数创建一个默认的 Wi-Fi Station 接口。
    // Wi-Fi Station 模式允许设备作为客户端连接到无线路由器或其他 Wi-Fi 热点。
    esp_netif_create_default_wifi_sta();

    // 3. 配置并初始化Wi-Fi驱动
    // 初始化配置：声明并初始化一个 wifi_init_config_t 结构体变量 cfg，使用 WIFI_INIT_CONFIG_DEFAULT() 宏来填充默认的初始化配置。
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    // 调用esp_wifi_init(&cfg) 根据上述配置初始化Wi-Fi驱动
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 4. 设置Wi-Fi参数
    // 通过 esp_wifi_set_storage() 函数指定 Wi-Fi 配置（如SSID和密码）存储在 RAM 中
    // 这意味着配置信息在设备重启后不会保留
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    // 设置Wi-Fi模式为 STA（Station） 模式，表明设备将作为客户端连接到Wi-Fi网络。
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // 5. 注册事件处理器，分别注册了两个事件处理器
    // 为所有Wi-Fi事件注册 wifi_event_handler 函数，这使得当任何 Wi-Fi 相关事件发生时，都会调用此处理函数。
    // a. ESP_ERROR_CHECK 宏：
    //      这是一个错误检查宏，它会检查括号内的函数调用返回值。
    //      如果返回值表示错误（非零），它将引发一个panic（通常导致程序停止运行）,并打印错误信息
    //      这确保了程序在遇到错误时能够及时响应。

    // b. esp_event_handler_register()：
    //      这是ESP-IDF（Espressif IoT Development Framework）中的一个函数
    //      它用于注册事件处理器
    //      它的主要作用是在事件管理系统中设置一个回调函数
    //      当特定事件发生时，系统会调用这个函数

    // c. WIFI_EVENT：
    //      这是事件类型的枚举值，表示 Wi-Fi 事件源
    //      这意味着注册的事件处理器将接收与 Wi-Fi 模块相关的事件
    //      如连接状态变化、SSID扫描完成等

    // d. ESP_EVENT_ANY_ID：
    //      这是事件ID的一个特殊值，表示匹配任何ID
    //      这意味着 wifi_event_handler 将被调用处理来自 WIFI_EVENT 源的所有可能的事件
    //      而不仅仅是特定的一个或几个事件

    // e. &wifi_event_handler：
    //      这是事件处理函数的指针
    //      当有匹配的事件发生时，ESP-IDF会调用这个函数。

    // f. NULL：
    //      这个参数通常用于传递一个上下文指针
    //      可以是一个结构体指针或其他数据
    //      以便在事件处理函数中访问额外的信息
    //      在这里，由于传递了NULL，事件处理函数将不会收到额外的上下文数据。

    // g. 总结：
    //      这段代码的作用是将 wifi_event_handler 函数注册为一个事件处理器，用于处理所有类型的WiFi事件
    //      当有WiFi事件发生时，esp_event_handler_register 确保 wifi_event_handler 会被正确调用
    //      并在出现错误时通过 ESP_ERROR_CHECK 宏进行错误处理。
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    // 为IP事件中的 IP_EVENT_STA_GOT_IP 事件注册了相同的处理函数，即当 Station 接口成功获取到IP地址时，会触发此处理器执行。
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    return ESP_OK;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "WiFi连接中…");
        led_control(LED_COLOR_GREEN, LED_MODE_BLINK_SLOW); // Blink green
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "WiFi已断开，正在重试…");
        esp_wifi_connect();
        led_control(LED_COLOR_ORANGE, LED_MODE_ON); // Solid orange if disconnected
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG, "WiFi连接成功！");
        led_control(LED_COLOR_GREEN, LED_MODE_ON); // Solid green
    }
}

static void connect_to_wifi(const char *ssid, const char *password)
{
    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    ESP_LOGI(TAG, "正在连接WiFi：%s", ssid);
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();
}

static void start_ap_mode()
{
    esp_netif_create_default_wifi_ap();

    wifi_config_t ap_config = {
        .ap = {
            .ssid = "ESP32_WIFI_SETUP",
            .ssid_len = strlen("ESP32_WIFI_SETUP"),
            // .password = 0,  // Change to your desired password
            .max_connection = 4,
            // .authmode = WIFI_AUTH_WPA_WPA2_PSK
            .authmode = WIFI_AUTH_OPEN},
    };

    // if (strlen(ap_config.ap.password) == 0) {
    //     ap_config.ap.authmode = WIFI_AUTH_OPEN;
    // }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "启用热点模式，连接WiFi：ESP32_WIFI_SETUP");
    // Start the web server to allow WiFi configuration via a web page
    start_web_server();
    ESP_LOGI(TAG, "Web服务器已启动");
}

static esp_err_t load_wifi_config(char *ssid, size_t ssid_size, char *password, size_t password_size)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("wifi_config", NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK)
        return ret;

    ret = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_size);
    if (ret != ESP_OK)
    {
        nvs_close(nvs_handle);
        return ret;
    }

    ret = nvs_get_str(nvs_handle, "password", password, &password_size);
    nvs_close(nvs_handle);

    return ret;
}

esp_err_t save_wifi_config(const char *ssid, const char *password)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("wifi_config", NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK)
        return ret;

    ret = nvs_set_str(nvs_handle, "ssid", ssid);
    if (ret != ESP_OK)
    {
        nvs_close(nvs_handle);
        return ret;
    }

    ret = nvs_set_str(nvs_handle, "password", password);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    return ret;
}

// Function to clear saved WiFi settings
esp_err_t clear_wifi_settings()
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Open NVS namespace for WiFi settings
    err = nvs_open("wifi_config", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }
    else
    {
        // Erase the key under which WiFi settings are saved
        err = nvs_erase_all(nvs_handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "清除WiFi设置失败%s", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "WiFi设置已清除");
        }

        // Commit changes
        err = nvs_commit(nvs_handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "提交改变失败：%s", esp_err_to_name(err));
        }

        // Close NVS handle
        nvs_close(nvs_handle);
    }

    return err;
}
