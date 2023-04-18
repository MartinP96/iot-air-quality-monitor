#include "measurement_collection.h"

/* Global variables */
QueueHandle_t measurement_queue;
QueueHandle_t measurement_queue_mqtt;

/* Local variables */
esp_mqtt_client_handle_t mqtt_client;

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
    measurement_queue_mqtt = xQueueCreate(10, sizeof(measurement_packet_st));

    data_packet.index = 0;
    data_packet.co2 = 0;
    data_packet.temperature = 0;
    data_packet.humidity = 0;
    init_sensors();

    while(1)
    {   
        get_measurements(&data_packet);
        xQueueSend(measurement_queue, &data_packet, portMAX_DELAY);
        
        if (mqtt_status.mqtt_connected_status)
        {
            xQueueSend(measurement_queue_mqtt, &data_packet, portMAX_DELAY);
        }
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
    unsigned long sys_time = 0;
    unsigned long hearth_beat_time = 0;
    unsigned long mqtt_hb_counter = 0;
    char hb_str[10];
    int state = 0;

    measurement_packet_st received_measurements;
    esp_mqtt_event_t received_value;

    mqtt_client = mqtt_ini_client();

    // JSON packet variables
    char *json_packet_str;
    cJSON *json_packet, *timestamp, *co2, *temp, *humidity;

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
                if(wifi_status.wifi_connected_status) {
                    mqtt_start_client(mqtt_client);
                    state = 1;
                }
                break;

                // Start MQTT publish measurements task
            case 1:
                if (mqtt_status.mqtt_connected_status) {

                    // Subscribe to topic
                    esp_mqtt_client_subscribe(mqtt_client, "porenta/martin_room/air_quality/data/parameters",0);

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

                // Publish measurements
                if (xQueueReceive(measurement_queue_mqtt, &received_measurements, 0 ) == pdPASS){

                    printf("index: %d \n temp: %f \n hum: %f \n co2: %d \n", received_measurements.index, received_measurements.temperature, received_measurements.humidity, received_measurements.co2);
                    // Build JSON packet
                    json_packet = cJSON_CreateObject();
                    timestamp = cJSON_CreateString("test");
                    co2 = cJSON_CreateNumber(received_measurements.co2);
                    temp = cJSON_CreateNumber(received_measurements.temperature);
                    humidity = cJSON_CreateNumber(received_measurements.humidity);
                    cJSON_AddItemToObject(json_packet, "timestamp", timestamp);
                    cJSON_AddItemToObject(json_packet, "co2", co2);
                    cJSON_AddItemToObject(json_packet, "temperature", temp);
                    cJSON_AddItemToObject(json_packet, "humidity", humidity);
                    json_packet_str = cJSON_Print(json_packet);
                    esp_mqtt_client_publish(mqtt_client, MEASUREMENTS_TOPIC, json_packet_str, 0, 1, 0);

                    free(json_packet_str);
                    cJSON_Delete(json_packet);
                }
                
                if (xQueueReceive(queue, &received_value, 0 ) == pdPASS){
                    printf("%d\n", received_value.data_len);
                    printf("%s\n", received_value.data);
                    printf("%s\n", received_value.topic);
                }

                if(!wifi_status.wifi_connected_status || !mqtt_status.mqtt_connected_status) {
                    state = 0;
                    mqtt_stop_client(mqtt_client);
                }
                break;

            default:
                break;
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL); // Brisemo task po koncu izvajanja
}


// TODO: Check why i get stack overflow error
void build_json_packet(measurement_packet_st* measurements_in, char** json_string)
{
    cJSON *json_packet, *timestamp, *co2, *temp, *humidity;
    printf("index: %d \n temp: %f \n hum: %f \n co2: %d \n", measurements_in->index, measurements_in->temperature, measurements_in->humidity, measurements_in->co2);

    // Build JSON packet
    json_packet = cJSON_CreateObject();
    timestamp = cJSON_CreateString("test");
    co2 = cJSON_CreateNumber(measurements_in->co2);
    temp = cJSON_CreateNumber(measurements_in->temperature);
    humidity = cJSON_CreateNumber(measurements_in->humidity);

    cJSON_AddItemToObject(json_packet, "timestamp", timestamp);
    cJSON_AddItemToObject(json_packet, "co2", co2);
    cJSON_AddItemToObject(json_packet, "temperature", temp);
    cJSON_AddItemToObject(json_packet, "humidity", humidity);

    //*json_string = cJSON_Print(json_packet);

    cJSON_Delete(json_packet);
    cJSON_Delete(timestamp);
    cJSON_Delete(co2);
    cJSON_Delete(temp);
    cJSON_Delete(humidity);
}
