#include "mhz19.h"


static  const uint8_t CommandRead[MHZ19_PACKET_LENGTH] = {
    0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
static  const uint8_t CommandEnableAutoBaseCalibration[MHZ19_PACKET_LENGTH] = {
    0xFF, 0x01, 0x79, 0xA0, 0x00, 0x00, 0x00, 0x00, 0xE6};
static  const uint8_t CommandDisableAutoBaseCalibration[MHZ19_PACKET_LENGTH] = {
    0xFF, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86};
static  const uint8_t CommandCalibrateToZeroPoint[MHZ19_PACKET_LENGTH] = {
    0xFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78};

const static uart_port_t uart_num = UART_NUM_2;
const static int uart_buffer_size = (1024 * 2);
QueueHandle_t static uart_queue;

// Initialize MHZ19 UART COMM

void mhz19_init(int tx_pin, int rx_pin)
{
     uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    QueueHandle_t uart_queue;
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));
}

// Function to read CO2 from sensor

int mhz19_read_co2(void)
{
    int co2 = -1;

    uint8_t response[MHZ19_PACKET_LENGTH];
    memset(response, 0, sizeof(response));
    uart_write_bytes(uart_num, CommandRead, MHZ19_PACKET_LENGTH);

    int length = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t*)&length));
    length = uart_read_bytes(uart_num, response, MHZ19_PACKET_LENGTH, 100); 

    int checksum = mhz19_calculate_checksum(response);

    if (response[0] == 0xff && response[1] == CommandRead[2] && response[8] == checksum) {
        int high = response[2];
        int low = response[3];
        co2 = ((256*high) + low);
    }

    return co2;
}

// Function to set measureing range

bool mhz19_set_measuring_range(uint16_t arg_range)
{
    uint8_t low = (uint8_t)(arg_range % 256);
    uint8_t high = (uint8_t)(arg_range / 256);
    uint8_t command[MHZ19_PACKET_LENGTH] = {0xFF, 0x01, 0x99, 0x00, 0x00,
                                   0x00, high, low,  0x00};

    command[8] = mhz19_calculate_checksum(command);
    
    return mhz19_send_command(command);
}


// Clibration functions

bool mhz19_enable_auto_calibration(void)
{
    return mhz19_send_command(CommandEnableAutoBaseCalibration);
}

bool mhz19_disable_auto_calibration(void)
{
    return mhz19_send_command(CommandDisableAutoBaseCalibration);
}

bool mhz19_calibrate_zero_point(void)
{
    return mhz19_send_command(CommandCalibrateToZeroPoint);
}


// Function to calculate Packet Checksum

static uint8_t mhz19_calculate_checksum(const uint8_t* arg_packet)
{
    uint8_t checksum = 0;

    for (size_t i = 1; i < MHZ19_PACKET_LENGTH - 1; i++) {
        checksum += arg_packet[i];
    }
    checksum = 255 - checksum;
    checksum++;

    return checksum;
}


// Function to send command over uart

static bool mhz19_send_command(const uint8_t *arg_cmd)
{
    uint8_t response[MHZ19_PACKET_LENGTH];
    memset(response, 0, sizeof(response));
    uart_write_bytes(uart_num, arg_cmd, MHZ19_PACKET_LENGTH);

    int length = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t*)&length));
    length = uart_read_bytes(uart_num, response, MHZ19_PACKET_LENGTH, 100); 

    uint8_t checksum = mhz19_calculate_checksum(response);
    if (response[0] != 0xff || response[1] != arg_cmd[2] || response[8] != checksum) {
        return false;
    }
    return true;
}

