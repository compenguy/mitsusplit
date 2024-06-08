/*  WiFi support code, station and softAP modes

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_check.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi.h"

#define ESP_STA_MAXIMUM_RETRY CONFIG_ESP_STA_MAXIMUM_RETRY

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_STA_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_STA_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define ESP_STA_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

#define ESP_SOFTAP_SSID_PREFIX CONFIG_ESP_SOFTAP_SSID_PREFIX
#define ESP_SOFTAP_PASSWORD_PREFIX CONFIG_ESP_SOFTAP_PASSWORD_PREFIX
#define ESP_SOFTAP_CHANNEL CONFIG_ESP_SOFTAP_CHANNEL
#define ESP_MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN
#define ESP_WIFI_SOFTAP_SAE_SUPPORT CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
static const char *TAG = "wifi";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;
static int sta_connected = 0;
static int sta_errored = 0;

static void sta_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_STA_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_sta_init(void)
{
    s_wifi_event_group = xEventGroupCreate();
    sta_connected = 0;
    sta_errored = 1;

    // Handled in app_main()
    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "Failed to initialize wifi for STA mode");

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &sta_event_handler,
                                                        NULL,
                                                        &instance_any_id),
                    TAG,
                    "Failed to register wifi STA mode event handler");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &sta_event_handler,
                                                        NULL,
                                                        &instance_got_ip),
                    TAG,
                    "Failed to register wifi STA mode event handler");

    wifi_config_t wifi_config = { 0 };
    ESP_RETURN_ON_ERROR(esp_wifi_get_config(WIFI_IF_STA, &wifi_config), TAG, "Failed to retrieve wifi configuration settings");
    if (strlen((char *)wifi_config.sta.ssid) == 0) {
      ESP_LOGE(TAG, "Unable to start wifi in STA mode: no configuration available");
      return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "Failed to set wifi mode to APSTA");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed to start wifi STA interface");

    ESP_LOGI(TAG, "wifi_sta_init finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        sta_connected = 1;
        sta_errored = 0;
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
    } else if (bits & WIFI_FAIL_BIT) {
        sta_connected = 0;
        sta_errored = 1;
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
    } else {
        sta_connected = 0;
        sta_errored = 1;
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    return ESP_OK;
}

esp_err_t wifi_sta_config_wpa2(const char *ssid, const char *psk)
{
    wifi_config_t wifi_config = {
      .sta = {
          .ssid = "",
          .password = "",
          /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
           * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
           * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
           * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
           */
          .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
          .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
          .sae_h2e_identifier = ESP_STA_H2E_IDENTIFIER,
      },
    };
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, psk, sizeof(wifi_config.sta.password) - 1);
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "Failed to configure wifi STA interface");
    return ESP_OK;
}

int wifi_sta_connected(void)
{
  return sta_connected;
}

int wifi_sta_errored(void)
{
  return sta_errored;
}
/* Wifi softAP SSID prefix is set via the project configuration menu.

   The actual SSID will include a suffix based on the MAC address.
*/
static uint8_t ap_ssid[32] = "";
static uint8_t ap_psk[64] = "";

static void softap_event_handler(void* arg, esp_event_base_t event_base,
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

static esp_err_t wifi_softap_init_creds(void)
{
    if (strnlen((const char *)ap_ssid, sizeof(ap_ssid)) == 0) {
        uint8_t baseMac[6] = {0};
        ESP_RETURN_ON_ERROR(esp_read_mac(baseMac, ESP_MAC_WIFI_SOFTAP), TAG, "Failed to read softAP MAC");
        snprintf((char *)ap_ssid, sizeof(ap_ssid), "%6s%2X%2X%2X%2X", ESP_SOFTAP_SSID_PREFIX, baseMac[0], baseMac[1], baseMac[2], baseMac[3]);
    }
    ESP_LOGI(TAG, "SSID %s", ap_ssid);
    if (strnlen((const char *)ap_psk, sizeof(ap_psk)) == 0) {
        uint8_t baseMac[6] = {0};
        ESP_RETURN_ON_ERROR(esp_read_mac(baseMac, ESP_MAC_WIFI_SOFTAP), TAG, "Failed to read softAP MAC");
        snprintf((char *)ap_psk, sizeof(ap_psk), "%6s%2X%2X%2X%2X", ESP_SOFTAP_PASSWORD_PREFIX, baseMac[0], baseMac[1], baseMac[2], baseMac[3]);
    }
    ESP_LOGI(TAG, "PSK %s", ap_psk);
    return ESP_OK;
}

esp_err_t wifi_softap_init(void)
{
    // Handled in app_main()
    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "Failed to initialize wifi for softAP mode");

    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &softap_event_handler,
                                                        NULL,
                                                        NULL),
                    TAG,
                    "Failed to register wifi STA mode event handler");

    wifi_config_t wifi_config = { 0 };
    ESP_RETURN_ON_ERROR(esp_wifi_get_config(WIFI_IF_AP, &wifi_config), TAG, "Failed to retrieve wifi configuration settings");
    if (strlen((char *)wifi_config.ap.ssid) == 0) {
      wifi_softap_init_creds();
      ESP_RETURN_ON_ERROR(wifi_softap_config((const char *)ap_ssid, (const char *)ap_psk), TAG, "Failed to set wifi AP configuration");
    }

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "Failed to set wifi mode to APSTA");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed to start wifi softAP interface");

    ESP_LOGI(TAG, "wifi_softap_init finished. SSID:%s password:%s channel:%d",
             wifi_config.ap.ssid, wifi_config.ap.password, ESP_SOFTAP_CHANNEL);
    return ESP_OK;
}

esp_err_t wifi_softap_config(const char *ssid, const char *psk)
{
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "",
            .ssid_len = 0,
            .channel = ESP_SOFTAP_CHANNEL,
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
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &wifi_config), TAG, "Failed to configure wifi softAP interface");
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

