#ifndef __app_prov_h__
#define __app_prov_h__

#include "esp_wifi.h"
esp_err_t wifi_init_softap(void);
esp_err_t wifi_enable_scanning(void);
esp_err_t wifi_get_ap_list(wifi_ap_record_t** ap_info, uint16_t* record_count, uint16_t* ap_count);

#endif // __app_prov_h__
