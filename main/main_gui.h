#ifndef MAIN_GUI_H
#define MAIN_GUI_H


/*********************
 *      INCLUDES
 *********************/

#include "lvgl.h"
#include "lvgl_helpers.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_timer.h"

#include "wifi_driver_utils.h"
#include "measurement_collection_task.h"
#include "mqtt_driver_utils.h"

/*********************
 *      DEFINES
 *********************/
#define LV_TICK_PERIOD_MS 1

/*********************
 *      TYPE DEFS
 *********************/
typedef struct {
    char ssid[32];
    char pass[32];
} gui_wifi_data_typ;

typedef struct {
    int32_t co2;
    float temperature;
    float humidity;
    int32_t voc;
    int32_t ozone;
} gui_measurement_packet;

typedef struct {
    int sampling_time;
} gui_system_settings;


/************************
 *   GLOBAL VARIABLES
 ***********************/
extern QueueHandle_t gui_refresh_queue;

/*********************
 *      FUNCTIONS
 *********************/

// Global functions
void T00_user_interface_task(void *pvParameter);
void T_mqtt_communication_task(void *pvParameter);

void gui_set_co2_meter_value(int32_t value,int32_t min, int32_t max);
void gui_set_voc_meter_value(int value);
void gui_set_ozone_meter_value(int value);

// Static functions
static void Create_TAB1(lv_obj_t *tab_ptr);
// static void Create_TAB2(lv_obj_t *tab_ptr);
static void Create_TAB3(lv_obj_t *tab_ptr);
static void gui_initialize(lv_color_t *buf1, lv_color_t *buf2);
static void lv_tick_task(void *arg);
void static gui_create_gui(void);
static void T02_01_wifi_scan_task(void *pvParameter);
static void T02_02_refresh_task(void *pvParameter);

// Event functions
static void wifi_enable_switch_event_handler(lv_obj_t * obj, lv_event_t event);
static void wifi_connection_select_event_handler(lv_obj_t * obj_dropdown, lv_event_t event);
static void wifi_keyboard_event_handler(lv_obj_t * obj, lv_event_t event);

#endif
