/* IOT Firmwavre template

    author: Martin P.
    date: 07.10.2022
    desc:
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mqtt_client.h"

#include "main_gui.h"
#include "wifi_driver_utils.h"
#include "mqtt_driver_utils.h"
#include "mhz19.h"
#include "DHT22.h"
#include "cJSON.h"

//ADC TEST
#include "driver/adc.h"
#include "esp_adc_cal.h"

// MQTT Topic definitions
#define SYS_TOPIC  "porenta/martin_room/air_quality/sys"
#define HEARTH_BEAT_TOPIC "porenta/martin_room/air_quality/sys/beat"
#define HEARTH_BEAT_PEARIOD_US 1000000
#define MEASUREMENTS_TOPIC "porenta/martin_room/air_quality/data/measurements"
#define JSON_PACKET_SIZE 200


// Sensors IO Pins
#define MHZ19_TX_PIN 17
#define MHZ19_RX_PIN 16
#define DHT22_INPUT_PIN 32

// Variables
QueueHandle_t sys_measurement_queue;

// Task definitions
void T02_communication_task(void *param);
void T01_measurement_task(void *param);

// Function definitions
void get_measurements(gui_measurement_packet *data_packet);

static esp_adc_cal_characteristics_t adc1_chars;

void app_main(void)
{
    // Initialize wifi module
    esp_err_t ret = nvs_flash_init();
    wifi_init();

    // Ini mqtt module
    esp_mqtt_client_handle_t mqtt_client;
    sys_measurement_queue = xQueueCreate(1, JSON_PACKET_SIZE);

    // Create tasks 
    xTaskCreatePinnedToCore(T00_user_interface_task, "gui_task", 1024*6, NULL, 0, NULL, 1);
    xTaskCreate(T01_measurement_task, "measurement_task", 1024*4, NULL, 0, NULL);
    xTaskCreate(T02_communication_task, "communication_task", 1024*2, &mqtt_client, 0, NULL);
}

// System task
void T02_communication_task(void *param) 
{
    unsigned long sys_time = 0;
    unsigned long hearth_beat_time = 0;
    unsigned long mqtt_hb_counter = 0;
    char hb_str[10];

    int state = 0;
    char received_value[JSON_PACKET_SIZE];
    memset(received_value, 0, sizeof(received_value));

	while(1)
	{
        // Get system time
        sys_time = esp_timer_get_time();
        
        // MQTT Control State Machine
        switch(state)
        {
            // Ini state
            case 0:
                mqtt_hb_counter = 0;
                if(wifi_status.wifi_connected_status == true) {
                    param = mqtt_app_start();
                    state = 1;
                }
            break;
            
            // Start MQTT publish measurements task
            case 1:
                if (mqtt_status.mqtt_connected_status) {
                    state = 2;
                }
            break;

            // MQTT publish data
            case 2:
                // Publish hearthbeat
                if ((sys_time - hearth_beat_time) > HEARTH_BEAT_PEARIOD_US)
                {
                    mqtt_hb_counter++;
                    sprintf(hb_str, "%d", mqtt_hb_counter);
                    esp_mqtt_client_publish(param, HEARTH_BEAT_TOPIC, hb_str, 0, 1, 0);
                    hearth_beat_time = sys_time;

                }
                // Publish measurements
                if (xQueueReceive(sys_measurement_queue, &received_value, 0 ) == pdPASS){
                    esp_mqtt_client_publish(param, MEASUREMENTS_TOPIC, received_value, 0, 1, 0);
                }
                // Disable MQTT when wifi disabled
                if (wifi_status.wifi_connected_status == false) {
                    mqtt_app_stop(param);
                    param = NULL;
                    state = 0;
                }
            break;
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL); // Brisemo task po koncu izvajanja
}

// Get Measurements Task 
void T01_measurement_task(void *param)
{
    gui_measurement_packet data_packet;

    char *json_packet_str;
    cJSON *json_packet, *timestamp, *co2, *temp, *humidity;

    // Ini sensors
    mhz19_init(MHZ19_TX_PIN,MHZ19_RX_PIN);
    mhz19_set_measuring_range(MHZ19_RANGE_3000);
    setDHTgpio(DHT22_INPUT_PIN);

    char measurements_json_string[JSON_PACKET_SIZE];

    while(1)
    {
        get_measurements(&data_packet);
        xQueueSend(gui_refresh_queue, &data_packet, portMAX_DELAY);
        printf("temp: %f \n hum: %f \n", data_packet.temperature, data_packet.humidity);

        // Send data to mqtt publish task
        if (mqtt_status.mqtt_connected_status)
        {
            // Build JSON packet
            json_packet = cJSON_CreateObject();
            timestamp = cJSON_CreateString("test");
            co2 = cJSON_CreateNumber(data_packet.co2);
            temp = cJSON_CreateNumber(data_packet.temperature);
            humidity = cJSON_CreateNumber(data_packet.humidity);
            cJSON_AddItemToObject(json_packet, "timestamp", timestamp);
            cJSON_AddItemToObject(json_packet, "co2", co2);
            cJSON_AddItemToObject(json_packet, "temperature", temp);
            cJSON_AddItemToObject(json_packet, "humidity", humidity);
            json_packet_str = cJSON_Print(json_packet);
            strcpy(measurements_json_string, json_packet_str);
            printf("JSON PACKET LEN: %d\n", sizeof(json_packet_str));
            xQueueSend(sys_measurement_queue,&measurements_json_string, portMAX_DELAY);
            free(json_packet_str);
            cJSON_Delete(json_packet);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL); // Brisemo task po koncu izvajanja
}


// Helper functions
void get_measurements(gui_measurement_packet *data_packet)
{
        // CO2
        data_packet->co2 = (int32_t)mhz19_read_co2();
        
        // Temp and hum
        int ret = readDHT();
        data_packet->humidity = getHumidity();
        data_packet->temperature = getTemperature();
}











































