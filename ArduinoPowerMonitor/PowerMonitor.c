#include <stdint.h>
#include <stdlib.h>

#include "Types.h"
#include "Abort.h"
#include "SoftwareTimer.h"
#include "ModBusCrc16.h"
#include "AsyncIo.h"
#include "PowerMonitor.h"

#define PZEM_DEVICE_ID 1
#define PZEM_READ_REGISTERS_COMMAND_ID 4
#define PZEM_VOLTAGE_REGISTER_ADDRESS_HIGH 0
#define PZEM_VOLTAGE_REGISTER_ADDRESS_LOW 0
#define PZEM_REGISTER_READ_COUNT_HIGH 0
#define PZEM_REGISTER_READ_COUNT_LOW 3
#define PZEM_REGISTER_READ_COUNT 3

#define PZEM_DEVICE_ID_OFFSET 0
#define PZEM_COMMAND_ID_OFFSET 1
#define PZEM_COMMAND_CRC_HIGH_OFFSET 7
#define PZEM_COMMAND_CRC_LOW_OFFSET 6

#define PZEM_ERROR_RESPONSE_ID 0x84

#define PZEM_RESPONSE_ID_OFFSET 1
#define PZEM_RESPONSE_BYTE_COUNT_OFFSET 2
#define PZEM_RESPONSE_VOLTAGE_HIGH_OFFSET 3
#define PZEM_RESPONSE_VOLTAGE_LOW_OFFSET 4
#define PZEM_RESPONSE_AMPERAGE_HIGH_HIGH_OFFSET 5
#define PZEM_RESPONSE_AMPERAGE_HIGH_LOW_OFFSET 6
#define PZEM_RESPONSE_AMPERAGE_LOW_HIGH_OFFSET 7
#define PZEM_RESPONSE_AMPERAGE_LOW_LOW_OFFSET 8
#define PZEM_RESPONSE_CRC_HIGH_OFFSET 10
#define PZEM_RESPONSE_CRC_LOW_OFFSET 9

static uint8_t get_power_request_with_crc[] =
{
    PZEM_DEVICE_ID,
    PZEM_READ_REGISTERS_COMMAND_ID,
    PZEM_VOLTAGE_REGISTER_ADDRESS_HIGH,
    PZEM_VOLTAGE_REGISTER_ADDRESS_LOW,
    PZEM_REGISTER_READ_COUNT_HIGH,
    PZEM_REGISTER_READ_COUNT_LOW,
    0x00, 0x00 // CRC high and low, calculated at runtime
};

static uint8_t get_power_response_with_crc[11];
static bool last_current_draw_detected;
static current_draw_change_handler_t current_draw_change_handler;

static void get_power_request_sent(void *completed);
static void get_power_response_header_received(void *completed);
static void get_power_response_length_received(void *completed);
static void get_power_response_data_received(void *completed);
static void get_power_response_error_received(void *completed);
static void get_power_timer_handler(void *parameters);

void power_monitor_init(current_draw_change_handler_t change_handler)
{
    last_current_draw_detected = false;
    current_draw_change_handler = NULL;

    uint16_t crc = crc16(get_power_request_with_crc, sizeof(get_power_request_with_crc) - sizeof(uint16_t));

    get_power_request_with_crc[PZEM_COMMAND_CRC_HIGH_OFFSET] = crc >> 8;
    get_power_request_with_crc[PZEM_COMMAND_CRC_LOW_OFFSET] = crc;
}

void power_monitor_begin(current_draw_change_handler_t change_handler)
{
    current_draw_change_handler = change_handler;

    timers_add_interrupts(get_power_timer_handler, NULL, 500, true);
}

bool is_current_draw_detected()
{
    return last_current_draw_detected;
}

void get_power_timer_handler(void *arguments)
{
    io_write(get_power_request_with_crc, sizeof(get_power_request_with_crc), get_power_request_sent, 100);
}

static void get_power_request_sent(void *completed)
{
    if (!completed)
    {
        return;
    }

    io_read(get_power_response_with_crc, 2, get_power_response_header_received, 100);
}

static void get_power_response_header_received(void *completed)
{
    if (!completed)
    {
        return;
    }

    if (get_power_response_with_crc[PZEM_DEVICE_ID_OFFSET] != PZEM_DEVICE_ID)
    {
        return;
    }

    switch (get_power_response_with_crc[PZEM_RESPONSE_ID_OFFSET])
    {
        case PZEM_READ_REGISTERS_COMMAND_ID:
            io_read(get_power_response_with_crc + 2, 1, get_power_response_length_received, 100);
            break;
        case PZEM_ERROR_RESPONSE_ID:
            io_read(get_power_response_with_crc + 2, 3, get_power_response_error_received, 100);
            break;
    }
}

static void get_power_response_length_received(void *completed)
{
    if (!completed)
    {
        return;
    }

    if (get_power_response_with_crc[PZEM_RESPONSE_BYTE_COUNT_OFFSET] == sizeof(uint16_t) * PZEM_REGISTER_READ_COUNT)
    {
        io_read(get_power_response_with_crc + 3, get_power_response_with_crc[PZEM_RESPONSE_BYTE_COUNT_OFFSET] + 2, get_power_response_data_received, 100);
    }
}

static void get_power_response_data_received(void *completed)
{
    if (!completed)
    {
        return;
    }

    uint16_t expected_crc = crc16(get_power_response_with_crc, sizeof(get_power_response_with_crc) - sizeof(uint16_t));

    uint16_t actual_crc =
        (get_power_response_with_crc[PZEM_RESPONSE_CRC_HIGH_OFFSET] << 8) |
        get_power_response_with_crc[PZEM_RESPONSE_CRC_LOW_OFFSET];

    if (actual_crc != expected_crc)
    {
        return;
    }

    uint16_t volt_reading =
        (get_power_response_with_crc[PZEM_RESPONSE_VOLTAGE_HIGH_OFFSET] << 8) |
        (get_power_response_with_crc[PZEM_RESPONSE_VOLTAGE_LOW_OFFSET] << 0);

    uint32_t amp_reading =
        ((uint32_t) get_power_response_with_crc[PZEM_RESPONSE_AMPERAGE_HIGH_HIGH_OFFSET] << 8) |
        ((uint32_t) get_power_response_with_crc[PZEM_RESPONSE_AMPERAGE_HIGH_LOW_OFFSET]) |
        (((uint32_t) get_power_response_with_crc[PZEM_RESPONSE_AMPERAGE_LOW_HIGH_OFFSET]) << 24) |
        (((uint32_t) get_power_response_with_crc[PZEM_RESPONSE_AMPERAGE_LOW_LOW_OFFSET]) << 16);

    bool current_draw_detected = (volt_reading > 1000 && volt_reading < 2500 && amp_reading > 3000);
    
    if (current_draw_detected != last_current_draw_detected)
    {
         last_current_draw_detected = current_draw_detected;
         current_draw_change_handler(current_draw_detected);
    }
}

static void get_power_response_error_received(void *completed)
{
    // Error information might be useful, but without
    // being debugged, there's no way to observe it.
}
