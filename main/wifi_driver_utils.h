
#ifndef _WIFI_DRIVER_UTILS_H_
#define _WIFI_DRIVER_UTILS_H_

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

/* Set the SSID and Password via project configuration, or can set directly here */
#define DEFAULT_SSID "IOT_Wifi"
#define DEFAULT_PWD "12602315"
#define DEFAULT_SCAN_METHOD WIFI_FAST_SCAN
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#define DEFAULT_RSSI -127
#define DEFAULT_AUTHMODE WIFI_AUTH_OPEN
#define DEFAULT_SCAN_LIST_SIZE 10




static const char *TAG_WIFI = "scan";

static void print_auth_mode(int authmode);
static void print_cipher_type(int pairwise_cipher, int group_cipher);
void event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data);
void wifi_connect(char arg_ssid[], char arg_password[]);
int wifi_scan(wifi_ap_record_t *ap_info);
void wifi_init(void);

#endif