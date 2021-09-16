/* mqtt - handles mqtt connectivity
 * SPDX-License-Identifier: CC0-1.0
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

#include <lwip/sockets.h>
#include <lwip/dns.h>
#include <lwip/netdb.h>

#include <mqtt_client.h>

#include "mitsusplit.h"

static const char *TAG = "mitsusplit_mqtt";
static const char *NVS_NS = "mqtt";

static void mqtt_set_connected(mqtt_context_t *context)
{
    if (context == NULL) { return; }

    context->state = MQTT_EVENT_CONNECTED;
}

static bool mqtt_connected(mqtt_context_t *context)
{
    if (context == NULL) { return false; }

    return context->state == MQTT_EVENT_CONNECTED;
}

static void mqtt_set_disconnected(mqtt_context_t *context)
{
    if (context == NULL) { return; }

    context->state = MQTT_EVENT_DISCONNECTED;
}

static bool mqtt_disconnected(mqtt_context_t *context)
{
    if (context == NULL) { return false; }

    return context->state == MQTT_EVENT_DISCONNECTED;
}

static void mqtt_set_errored(mqtt_context_t *context)
{
    if (context == NULL) { return; }

    context->state = MQTT_EVENT_ERROR;
}

static bool mqtt_errored(mqtt_context_t *context)
{
    if (context == NULL) { return false; }

    return context->state == MQTT_EVENT_ERROR;
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        mqtt_set_connected((mqtt_context_t*)handler_args);
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        mqtt_set_disconnected((mqtt_context_t*)handler_args);
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        mqtt_set_errored((mqtt_context_t*)handler_args);
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            if (event->error_handle->esp_tls_last_esp_err) { ESP_LOGI(TAG, "reported from esp-tls"); }
            if (event->error_handle->esp_tls_stack_err) { ESP_LOGI(TAG, "reported from tls stack"); }
            if (event->error_handle->esp_transport_sock_errno) { ESP_LOGI(TAG, "captured as transport's socket errno"); }
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void init_mqtt(mqtt_context_t *context)
{
    if (context == NULL) { return; }
    context->state = MQTT_EVENT_DISCONNECTED;
    context->session = NULL;
}

void start_mqtt(mqtt_context_t *context)
{
    if (context == NULL) { return; }

    if (mqtt_connected(context) || mqtt_errored(context))
    {
        ESP_LOGI(TAG, "stopping connected or errored mqtt session (%d) before completing start request", context->state);
        stop_mqtt(context);
    }

    nvs_handle_t nvs_mqtt;
    ESP_ERROR_CHECK(nvs_open(NVS_NS, NVS_READWRITE, &nvs_mqtt));
    char broker[40];
    size_t len = 0;
    esp_err_t err = nvs_get_str(nvs_mqtt, "broker", broker, &len);
    if (len >= sizeof(broker))
    {
        ESP_LOGE(TAG, "broker URI too long for buffer, aborting connection");
        return;
    }
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(TAG, "no broker configured, not starting broker");
        return;
    }
    ESP_ERROR_CHECK(err);

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = broker
    };

    context->session = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(context->session, ESP_EVENT_ANY_ID, mqtt_event_handler, context);
    esp_mqtt_client_start(context->session);
}

void stop_mqtt(mqtt_context_t *context)
{
    if (context == NULL) { return; }
    if (context->session)
    {
        ESP_ERROR_CHECK(esp_mqtt_client_stop(context->session));
    }
    init_mqtt(context);
}
