#include "esp_http_server.h"
#include "esp_log.h"
#include <ctype.h> // Include this for isxdigit and other character functions
#include <stdlib.h>
#include <string.h>
#include "wifi_manager.h" // To use functions for saving new WiFi settings
#include "nvs_flash.h"

static const char *TAG = "WEB_SERVER";

// TODO: 保存wifi配置后，提示保存成功（对话框或者通知），地址要回到主页，不要停留在提交的地址

/* HTML Form for entering WiFi credentials */
const char *html_form =
    "<!DOCTYPE html>"
    "<html lang=\"zh-CN\">"
    "<head>"
    "<meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
    "<title>设备配置页面</title>"
    "<style>"
    "body {"
    "font-family: Arial, sans-serif;"
    "margin: 0;"
    "padding: 0;"
    "display: flex;"
    "justify-content: center;"
    "align-items: center;"
    "height: 100vh;"
    "background-color: #f0f0f0;"
    "}"
    ".container {"
    "background-color: #fff;"
    "padding: 20px;"
    "border-radius: 10px;"
    "box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);"
    "width: 90%%;"
    "max-width: 400px;"
    "}"
    "h2 {"
    "text-align: center;"
    "color: #333;"
    "}"
    "form {"
    "display: flex;"
    "flex-direction: column;"
    "}"
    "label {"
    "margin-bottom: 10px;"
    "font-weight: bold;"
    "color: #555;"
    "}"
    "input[type=\"text\"],"
    "input[type=\"password\"] {"
    "padding: 10px;"
    "margin-bottom: 20px;"
    "border: 1px solid #ccc;"
    "border-radius: 5px;"
    "font-size: 16px;"
    "}"
    ".button-container {"
    "display: flex;"
    "justify-content: flex-end;"
    "}"
    "button {"
    "padding: 10px 20px;"
    "border: none;"
    "border-radius: 5px;"
    "background-color: #007BFF;"
    "color: white;"
    "font-size: 16px;"
    "cursor: pointer;"
    "}"
    "button:hover {"
    "background-color: #0069d9;"
    "}"
    "</style>"
    "</head>"
    "<body>"
    "<div class=\"container\">"
    "<h2>WiFi 配置</h1>"
    "<form action=\"/submit\" method=\"post\">"
    "<label for=\"ssid\">WiFi 账号</label>"
    "<input type=\"text\" id=\"ssid\" name=\"ssid\" required>"
    "<label for=\"password\">WiFi 密码</label>"
    "<input type=\"password\" id=\"password\" name=\"password\" required>"
    "<div class=\"button-container\">"
    "<button type=\"submit\">保存</button>"
    "</div>"
    "</form>"
    "</div>"
    "</body>"
    "</html>";

/* URL Decoding function */
static void url_decode(char *dst, const char *src)
{
    char a, b;
    while (*src)
    {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b)))
        {
            if (a >= 'a')
                a -= 'a' - 'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a' - 'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';

            *dst++ = 16 * a + b;
            src += 3;
        }
        else if (*src == '+')
        {
            *dst++ = ' ';
            src++;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}
esp_err_t get_handler(httpd_req_t *req)
{
    httpd_resp_send(req, html_form, strlen(html_form));
    return ESP_OK;
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

esp_err_t post_handler(httpd_req_t *req)
{
    char content[100], decoded_content[100];
    int received = httpd_req_recv(req, content, sizeof(content) - 1);
    if (received <= 0)
    { // Check for error or no data received
        return ESP_FAIL;
    }
    content[received] = '\0';

    // Decode URL-encoded content
    url_decode(decoded_content, content);

    // Parse ssid and password
    char ssid[32], password[64];
    sscanf(decoded_content, "ssid=%31[^&]&password=%63s", ssid, password);

    ESP_LOGI(TAG, "收到路由器WiFi名称：%s，密码：%s", ssid, password);

    // 尝试按照新的WiFi配置连接
    // 连接不成功，返回WiFi密码错误提示
    // 连接成功，保存新的WiFi配置
    // 重启设备
    // const char *resp = "";
    // if (check_wifi_config(ssid, password) == ESP_OK)
    // {
    //     save_wifi_config_to_nvs(ssid, password);
    //     // ESP_LOGI(TAG, "WiFi设置成功，重启设备");
    //     resp = "<html><head><meta charset=\"UTF-8\"></head><body><script>"
    //            "alert('WiFi配置保存成功');"
    //            "window.location.href='/';"
    //            "</script></body></html>";
    //     esp_restart(); // 重启设备
    // }
    // else
    // {
    //     resp = "<html><head><meta charset=\"UTF-8\"></head><body><script>"
    //            "alert('WiFi密码错误，请重新输入');"
    //            "window.location.href='/';"
    //            "</script></body></html>";
    // }
    // httpd_resp_set_type(req, "text/html");
    // httpd_resp_send(req, resp, strlen(resp));

    // Save the new credentials using the function declared in wifi_manager.h
    save_wifi_config_to_nvs(ssid, password);

    // Respond with a success message and redirect to the main page
    const char *resp = "<html><body><script>"
                       "alert('WiFi配置保存成功，按确定将重启设备');"
                       "window.location.href='/';"
                       "</script></body></html>";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp, strlen(resp));
    esp_restart(); // Restart the device

    return ESP_OK;
}

esp_err_t start_web_server(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the server
    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_uri_t get_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = get_handler,
            .user_ctx = NULL};
        httpd_uri_t post_uri = {
            .uri = "/submit",
            .method = HTTP_POST,
            .handler = post_handler,
            .user_ctx = NULL};

        // Register URI handlers
        httpd_register_uri_handler(server, &get_uri);
        httpd_register_uri_handler(server, &post_uri);
    }

    return ESP_OK;
}
