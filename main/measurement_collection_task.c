#include "measurement_collection_task.h"

/**
 * Desc: Initialize used sensors
 * 
 * Param: /
 * Return: /
 */
void init_sensors(void)
{
    mhz19_init(MHZ19_TX_PIN,MHZ19_RX_PIN);
    mhz19_set_measuring_range(MHZ19_RANGE_3000);
    setDHTgpio(DHT22_INPUT_PIN);
}

/**
 * Desc: Read sensor data
 * 
 * Param: 
 *  -   measurement_packet_st(data_packet): Pointer to the input data packet
 * 
 * Return: /
 */
void get_measurements(measurement_packet_st *data_packet)
{
    data_packet->index++;
    data_packet->co2 = (int32_t)mhz19_read_co2();

    int ret = readDHT();
    data_packet->humidity = getHumidity();
    data_packet->temperature = getTemperature();
}

/**
 * Desc: Measurement collection main task
 * 
 * Param: /
 * Return: /
 */
void T_measurement_task(void *param)
{
    measurement_packet_st data_packet;
    data_packet.index = 0;
    data_packet.co2 = 0;
    data_packet.temperature = 0;
    data_packet.humidity = 0;

    init_sensors();

    while(1)
    {   
        get_measurements(&data_packet);
        printf("index: %d \n temp: %f \n hum: %f \n co2: %d \n", data_packet.index, data_packet.temperature, data_packet.humidity, data_packet.co2);
        vTaskDelay(SAMPLING_TIME/ portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL); // Brisemo task po koncu izvajanja
}