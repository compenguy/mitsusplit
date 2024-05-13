/*  WiFi provisioning softAP

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_check.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "app_prov.h"

#define ESP_WIFI_SSID_PREFIX CONFIG_ESP_WIFI_SSID_PREFIX
#define ESP_WIFI_PASSWORD_PREFIX CONFIG_ESP_WIFI_PASSWORD_PREFIX
#define ESP_WIFI_CHANNEL CONFIG_ESP_WIFI_CHANNEL
#define ESP_MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN
#define ESP_WIFI_SOFTAP_SAE_SUPPORT CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
static const char *TAG = "wifi softAP";

/* Wifi softAP SSID prefix is set via the project configuration menu.

   The actual SSID will include a suffix based on the MAC address.
*/
static uint8_t ap_ssid[16] = "";
static uint8_t ap_psk[36] = "";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

static esp_err_t wifi_init_creds(void)
{
    if (strnlen((const char *)ap_ssid, sizeof(ap_ssid)) == 0) {
        uint8_t baseMac[6] = {0};
        ESP_RETURN_ON_ERROR(esp_read_mac(baseMac, ESP_MAC_WIFI_SOFTAP), TAG, "Failed to read softAP MAC");
        snprintf((char *)ap_ssid, sizeof(ap_ssid), "%6s%2X%2X%2X%2X", ESP_WIFI_SSID_PREFIX, baseMac[0], baseMac[1], baseMac[2], baseMac[3]);
    }
    ESP_LOGI(TAG, "SSID %s", ap_ssid);
    if (strnlen((const char *)ap_psk, sizeof(ap_psk)) == 0) {
        uint8_t baseMac[6] = {0};
        ESP_RETURN_ON_ERROR(esp_read_mac(baseMac, ESP_MAC_WIFI_SOFTAP), TAG, "Failed to read softAP MAC");
        snprintf((char *)ap_psk, sizeof(ap_psk), "%6s%2X%2X%2X%2X", ESP_WIFI_PASSWORD_PREFIX, baseMac[0], baseMac[1], baseMac[2], baseMac[3]);
    }
    ESP_LOGI(TAG, "PSK %s", ap_psk);
    return ESP_OK;
}

esp_err_t wifi_init_softap(void)
{
    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_init_creds();
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "",
            .ssid_len = 0,
            .channel = ESP_WIFI_CHANNEL,
            .password = "",
            .max_connection = ESP_MAX_STA_CONN,
#ifdef ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* ESP_WIFI_SOFTAP_SAE_SUPPORT */
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                    .required = true,
            },
        },
    };
    // Initialize the ssid and password members
    memcpy(wifi_config.ap.ssid, ap_ssid, sizeof(ap_ssid));
    wifi_config.ap.ssid_len = strlen((const char *)ap_ssid);

    memcpy(wifi_config.ap.password, ap_psk, sizeof(ap_psk));

    if (strlen((const char *)ap_psk) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             ap_ssid, ap_psk, ESP_WIFI_CHANNEL);
    return ESP_OK;
}

/* Simple handler for getting access points */
static bool scanning = false;
esp_err_t wifi_enable_scanning(void)
{
    if (!scanning) {
        ESP_RETURN_ON_ERROR(esp_wifi_scan_start(NULL, false), TAG, "Failed to start wifi network scanning");
        scanning = true;
    }
    return ESP_OK;
}

