#ifndef __mitsusplit_h__
#define __mitsusplit_h__

#include <freertos/event_groups.h>
#include <mqtt_client.h>

// Context structure for tracking provisioning state and data
typedef struct provisioning_context
{
    EventGroupHandle_t wifi_event_group;
} provisioning_context_t;

// Initialize the provisioning_context_t data structure for use
void init_provisioning(provisioning_context_t *);

// Asynchronous (non-blocking) function for provisioning wifi, then connecting to
// wifi
//
// Aborts on error
void start_provisioning(provisioning_context_t *);

// Block until provisioning completed and wifi connected
void wait_provisioned(provisioning_context_t *);

// Context structure for tracking mqtt state and data
typedef struct mqtt_context
{
    esp_mqtt_event_id_t state;
    esp_mqtt_client_handle_t session;
} mqtt_context_t;

// Initialize the mqtt_context_t data structure for use
void init_mqtt(mqtt_context_t *);

// Synchronous (blocking) function for starting mqtt client, connecting to
// broker and returning (with subsequent asynchronous mqtt message handling)
//
// Aborts on error
void start_mqtt(mqtt_context_t *);

// Synchronous (blocking) function for stopping mqtt client, and stopping the
// event thread
void stop_mqtt(mqtt_context_t *);

#endif // __mitsusplit_h__
