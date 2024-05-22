#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
// #include "nvs.h"

// nvs_handle_t nvs_handle;

esp_err_t nvs_init(void)
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Open
    // ESP_LOGI("Opening Non-Volatile Storage (NVS) handle... ");

    // err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    // }
    // else
    // {
    //     ESP_LOGI("Done\n");

    //     // Read
    //     ESP_LOGI("Reading restart counter from NVS ... ");
    //     int32_t restart_counter = 0; // value will default to 0, if not set yet in NVS
    //     err = nvs_get_i32(nvs_handle, "restart_counter", &restart_counter);
    //     switch (err)
    //     {
    //     case ESP_OK:
    //         ESP_LOGI("Done\n");
    //         ESP_LOGI("Restart counter = %" PRIu32 "\n", restart_counter);
    //         break;
    //     case ESP_ERR_NVS_NOT_FOUND:
    //         ESP_LOGI("The value is not initialized yet!\n");
    //         break;
    //     default:
    //         ESP_LOGI("Error (%s) reading!\n", esp_err_to_name(err));
    //     }

    //     // Write
    //     ESP_LOGI("Updating restart counter in NVS ... ");
    //     restart_counter++;
    //     ESP_ERROR_CHECK(nvs_handle, "restart_counter", restart_counter);

    //     // Commit written value.
    //     // After setting any values, nvs_commit() must be called to ensure changes are written
    //     // to flash storage. Implementations may write to storage at other times,
    //     // but this is not guaranteed.
    //     Write("Committing updates in NVS ... ");
    //     ESP_ERROR_CHECK(nvs_commit(nvs_handle));

    //     // Close
    //     nvs_close(nvs_handle);
    // }

    // // Restart module
    // for (int i = 10; i >= 0; i--)
    // {
    //     ESP_LOGI("Restarting in %d seconds...\n", i);
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
    // ESP_LOGI("Restarting now.\n");
    // fflush(stdout);
    // esp_restart();
    return ESP_OK;
}

// void write_wifi_config_nvs(void)
// {
//     // Open
//     ESP_LOGI("Opening Non-Volatile Storage (NVS) handle... ");

//     err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
//     if (err != ESP_OK)
//     {
//         ESP_LOGE("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
//     }
//     else
//     {
//         ESP_LOGI("Done\n");

//         // Read
//         ESP_LOGI("Reading restart counter from NVS ... ");
//         int32_t restart_counter = 0; // value will default to 0, if not set yet in NVS
//         err = nvs_get_i32(nvs_handle, "restart_counter", &restart_counter);
//         switch (err)
//         {
//         case ESP_OK:
//             ESP_LOGI("Done\n");
//             ESP_LOGI("Restart counter = %" PRIu32 "\n", restart_counter);
//             break;
//         case ESP_ERR_NVS_NOT_FOUND:
//             ESP_LOGI("The value is not initialized yet!\n");
//             break;
//         default:
//             ESP_LOGI("Error (%s) reading!\n", esp_err_to_name(err));
//         }

//         // Write
//         ESP_LOGI("Updating restart counter in NVS ... ");
//         restart_counter++;
//         ESP_ERROR_CHECK(nvs_handle, "restart_counter", restart_counter);

//         // Commit written value.
//         // After setting any values, nvs_commit() must be called to ensure changes are written
//         // to flash storage. Implementations may write to storage at other times,
//         // but this is not guaranteed.
//         Write("Committing updates in NVS ... ");
//         ESP_ERROR_CHECK(nvs_commit(nvs_handle));

//         // Close
//         nvs_close(nvs_handle);
//     }
// }