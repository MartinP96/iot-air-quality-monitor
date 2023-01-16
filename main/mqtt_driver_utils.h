#ifndef MQTT_DRIVER_UTILS_H
#define MQTT_DRIVER_UTILS_H

#include "mqtt_client.h"
#include "esp_log.h"

extern QueueHandle_t queue;

// Type Defs
typedef struct {
    bool mqtt_connected_status;
} mqtt_connection_status;


//Global variables
extern mqtt_connection_status mqtt_status;


//Function definitions
void log_error_if_nonzero(const char *message, int error_code);
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
esp_mqtt_client_handle_t mqtt_app_start(void);
char* substring(char *destination, const char *source, int beg, int n);
void mqtt_app_stop(esp_mqtt_client_handle_t client);

#endif