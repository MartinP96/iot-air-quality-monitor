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

void app_main(void)
{
    // Initialize wifi module
    esp_err_t ret = nvs_flash_init();
    wifi_init();

    // Create tasks 
    xTaskCreatePinnedToCore(T00_user_interface_task, "gui_task", 1024*6, NULL, 0, NULL, 1);
}
