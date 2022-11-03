#ifndef _MHZ19_H_
#define _MHZ19_H_

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "driver/uart.h"
#include "driver/gpio.h"


#define MHZ19_PACKET_LENGTH 9
#define MHZ19_RANGE_1000  1000
#define MHZ19_RANGE_2000  2000
#define MHZ19_RANGE_3000  3000
#define MHZ19_RANGE_5000  5000


// Public function

void mhz19_init(int tx_pin, int rx_pin);
int mhz19_read_co2(void);
bool mhz19_set_measuring_range(uint16_t arg_range);

bool mhz19_enable_auto_calibration(void); // TODO
bool mhz19_disable_auto_calibration(void); // TODO
bool mhz19_calibrate_zero_point(void); // TODO
bool mhz19_calibrate_span_point(uint16_t spanPoint); // TODO
// Static functions

static bool mhz19_send_command(const uint8_t *arg_cmd); 
static uint8_t mhz19_calculate_checksum(const uint8_t* packet);


#endif