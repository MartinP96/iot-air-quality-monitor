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

#include "system_gui.h"
#include "wifi_driver_utils.h"
#include "mqtt_driver_utils.h"

#include "mhz19.h"


typedef struct {
    bool wifi_connected_status;
    bool mqtt_connected_status;
    char wifi_ssid[32];
    char wifi_pass[32];
} sys_status;

sys_status system_status;

// Task definitions
void T00_sys_task(void *param);
void T01_measurement_task(void *param);

void app_main(void)
{
    // Initialize wifi module
    esp_err_t ret = nvs_flash_init();
    wifi_init();

    // Ini mqtt client
    //esp_mqtt_client_handle_t client = mqtt_app_start();

    // Create tasks 
    //xTaskCreatePinnedToCore(T00_sys_task, "sys_task", 1024*2, client, 0, NULL, 0);
    xTaskCreatePinnedToCore(T02_user_interface_task, "gui_task", 1024*4, NULL, 0, NULL, 1);
    xTaskCreatePinnedToCore(T01_measurement_task, "measurement_task", 1024*2, NULL, 3, NULL,0);
}

// System task
void T00_sys_task(void *param) 
{
    unsigned long sys_time = 0;
    unsigned long hearth_beat_time = 0;
    int msg_id;

	while(1)
	{
        // Get system time
        sys_time = esp_timer_get_time();
        
        // Send heatbit to mqtt broker
        //msg_id = esp_mqtt_client_publish(param, HEARTH_BEAT_TOPIC, "hbeat", 0, 1, 0);
        //hearth_beat_time = sys_time;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL); // Brisemo task po koncu izvajanja
}

// Get Measurements Task (TEST)
void T01_measurement_task(void *param)
{
    mhz19_init(17,16);
    mhz19_set_measuring_range(MHZ19_RANGE_3000);

    while(1)
    {
        int co2 = mhz19_read_co2();
        printf("co2: %d\n", co2);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL); // Brisemo task po koncu izvajanja
}
