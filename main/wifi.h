#ifndef __wifi_h__
#define __wifi_h__

#include "esp_wifi.h"
esp_err_t wifi_enable_scanning(void);
esp_err_t wifi_get_ap_list(wifi_ap_record_t** ap_info, uint16_t* record_count, uint16_t* ap_count);
esp_err_t wifi_sta_init(void);
esp_err_t wifi_sta_config_wpa2(const char *ssid, const char *psk);
int wifi_sta_connected(void);
int wifi_sta_errored(void);

esp_err_t wifi_softap_init(void);
esp_err_t wifi_softap_config(const char *ssid, const char *psk);


#endif // __wifi_h__
