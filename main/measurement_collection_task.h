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

/*********************
 *      DEFINES
 *********************/

#define SAMPLING_TIME 1000 //in ms
#define MHZ19_TX_PIN 17
#define MHZ19_RX_PIN 16
#define DHT22_INPUT_PIN 32


/*********************
 *      TYPE DEFS
 *********************/
typedef struct {
    int32_t co2;
    float temperature;
    float humidity;

} measurement_packet_st;


/************************
 *   GLOBAL VARIABLES
 ***********************/

/*********************
 *      FUNCTIONS
 *********************/

void T_measurement_task(void *param);
static void get_measurements(measurement_packet_st *data_packet);
static void init_sensors(void);

#endif