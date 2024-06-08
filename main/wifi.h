#ifndef __wifi_h__
#define __wifi_h__

#include "esp_wifi.h"
esp_err_t wifi_enable_scanning(void);
esp_err_t wifi_get_ap_list(wifi_ap_record_t** ap_info, uint16_t* record_count, uint16_t* ap_count);
esp_err_t wifi_softap_init(void);

#endif // __wifi_h__
