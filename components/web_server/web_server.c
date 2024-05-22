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
    "<title>Configure WiFi</title>"
    "<link href=\"https://cdn.bootcdn.net/ajax/libs/twitter-bootstrap/5.3.3/css/bootstrap.min.css\" rel=\"stylesheet\">"
    "</head>"
    "<body>"
    "<div class=\"container text-center\">"
    "<div class=\"row\">"
    "<h1 class=\"text-center\">WiFi设置</h1>"
    "</div>"
    "<div class=\"row justify-content-md-center\">"
    "<form class=\"col\" method=\"post\" action=\"/submit\">"
    "<div class=\"mb-3 row\">"
    "<lable for=\"ssid\" class=\"col-sm-2 col-form-label\">WiFi名称:</lable>"
    "<div class=\"col-sm-10\">"
    "<input type=\"text\" class=\"form-control\" id=\"ssid\" name=\"ssid\" placeholder=\"WiFi 名称\">"
    "</div>"
    "</div>"
    "<div class=\"mb-3 row\">"
    "<lable for=\"password\" class=\"col-sm-2 col-form-label\">WiFi密码:</lable>"
    "<div class=\"col-sm-10\">"
    "<input type=\"password\" class=\"form-control\" id=\"password\" name=\"password\" placeholder=\"WiFi 密码\">"
    "</div>"
    "</div>"
    "<div class=\"mb-3 row\">"
    "<input class=\"btn btn-primary\" type=\"submit\" value=\"保存\">"
    "</div>"
    "</form>"
    "</div>"
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
