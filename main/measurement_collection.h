#ifndef MEASUREMENT_COLLECTION_TASK_H
#define MEASUREMENT_COLLECTION_TASK_H

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_timer.h"

// Custom includes
#include "mhz19.h"
#include "DHT22.h"
#include "mqtt_driver_utils.h"
#include "wifi_driver_utils.h"
#include "cJSON.h"

/*********************
 *      DEFINES
 *********************/

#define SAMPLING_TIME 5000 //in ms
#define MHZ19_TX_PIN 17
#define MHZ19_RX_PIN 16
#define DHT22_INPUT_PIN 32


/*********************
 *      TYPE DEFS
 *********************/
typedef struct {
    int32_t index;
    int32_t co2;
    float temperature;
    float humidity;
} measurement_packet_st;


/************************
 *   GLOBAL VARIABLES
 ***********************/
extern QueueHandle_t measurement_queue;

/*********************
 *      FUNCTIONS
 *********************/
void T_measurement_task(void *param);
void T_mqtt_communication_task(void *pvParameter);
void get_measurements(measurement_packet_st *data_packet);
void init_sensors(void);
void T_mqtt_publish_task(void *pvParameter);
void T_mqtt_subscribe_task(void *pvParameter);
void build_json_packet(measurement_packet_st* measurements_in, char **json_string);

#endif