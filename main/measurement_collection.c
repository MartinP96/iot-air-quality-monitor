#include "measurement_collection.h"

/* Global variables */
QueueHandle_t measurement_queue;

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

    measurement_queue = xQueueCreate(10, sizeof(measurement_packet_st));

    data_packet.index = 0;
    data_packet.co2 = 0;
    data_packet.temperature = 0;
    data_packet.humidity = 0;

    init_sensors();

    while(1)
    {   
        get_measurements(&data_packet);
        xQueueSend(measurement_queue, &data_packet, portMAX_DELAY);
        printf("index: %d \n temp: %f \n hum: %f \n co2: %d \n", data_packet.index, data_packet.temperature, data_packet.humidity, data_packet.co2); 
        vTaskDelay(SAMPLING_TIME/ portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL); // Brisemo task po koncu izvajanja
}

/**
 * Desc: Measurement collection main task
 *
 * Param: /
 * Return: /
 */
// MQTT Communication task
void T_mqtt_communication_task(void *pvParameter)
{
    esp_mqtt_client_handle_t mqtt_client;

    unsigned long sys_time = 0;
    unsigned long hearth_beat_time = 0;
    unsigned long mqtt_hb_counter = 0;
    char hb_str[10];
    int state = 0;
    char received_value[JSON_PACKET_SIZE];
    memset(received_value, 0, sizeof(received_value));
    mqtt_client = mqtt_ini_client();

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
                    mqtt_start_client(mqtt_client);
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
                    esp_mqtt_client_publish(mqtt_client, HEARTH_BEAT_TOPIC, hb_str, 0, 1, 0);
                    hearth_beat_time = sys_time;
                }
                break;

            default:
                break;
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL); // Brisemo task po koncu izvajanja
}