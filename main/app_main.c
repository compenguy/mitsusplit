/* mitsusplit - mitsubishi mini-split control application
 * SPDX-FileCopyrightText: 2021 William Page <compenguy@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
*/

#include <stdio.h>
#include <sdkconfig.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <esp_system.h>
#include <esp_spi_flash.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_event.h>

#include "mitsusplit.h"

static const char* TAG = "mitsusplit";

// TODO: Assert (static assert?) required sdkconfig options for application

typedef struct app_context
{
    mqtt_context_t mqtt;
    provisioning_context_t provisioning;
} app_context_t;

void app_init(app_context_t *context)
{
    if (context == NULL) { return; }

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "This is %s chip with %d CPU core(s), WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    ESP_LOGI(TAG, "silicon revision %d, ", chip_info.revision);

    ESP_LOGI(TAG, "%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    init_mqtt(&context->mqtt);
    init_provisioning(&context->provisioning);
}

void app_main(void)
{
    app_context_t app_ctx;
    app_init(&app_ctx);

    start_provisioning(&app_ctx.provisioning);
    wait_provisioned(&app_ctx.provisioning);

    start_mqtt(&app_ctx.mqtt);

    /* Start main application now */
    while (1) {
        ESP_LOGI(TAG, "Hello World!");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
