#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <avr/pgmspace.h>

#include "Application.h"
#include "Types.h"
#include "ModBusCrc16.h"
#include "AsyncIo.h"
#include "Thread.h"
#include "PowerOut.h"

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
    0x00, 0x00 // CRC low and high, calculated at runtime
};

static uint8_t get_power_response_with_crc[11];
static int8_t last_current_draw_detected;

static void get_power_reading();
static void get_power_response();
static void get_error_response();

__attribute__((OS_task))
void thread_root_entry(void *unused_arguments)
{
#ifdef TEST
    static const __flash char string_power_monitor[] PROGMEM = "Power Monitor!";

    static volatile bool trueFlag = true;

    char status[sizeof(string_power_monitor) - 1];

    memcpy_P(status, string_power_monitor, sizeof(status));
#endif

    power_out_initialize();
    io_task_initialize();

    last_current_draw_detected = -1;

    uint16_t crc = crc16(get_power_request_with_crc, sizeof(get_power_request_with_crc) - sizeof(uint16_t));

    get_power_request_with_crc[PZEM_COMMAND_CRC_HIGH_OFFSET] = crc >> 8;
    get_power_request_with_crc[PZEM_COMMAND_CRC_LOW_OFFSET] = crc;

#ifdef TEST
    while (trueFlag)
#else
    for (;;)
#endif
    {
        thread_sleep(500);

        get_power_reading();
    }

#ifdef TEST
    memset(status, 0, sizeof(status));
#endif

    abort();
}

static void get_power_reading()
{
    io_write(get_power_request_with_crc, sizeof(get_power_request_with_crc), 0);

    if (io_read(get_power_response_with_crc, 2, 100) == -1)
    {
        return;
    }

    if (get_power_response_with_crc[PZEM_DEVICE_ID_OFFSET] != PZEM_DEVICE_ID)
    {
        return;
    }

    get_power_response_with_crc[PZEM_RESPONSE_ID_OFFSET] = PZEM_READ_REGISTERS_COMMAND_ID;

    switch (get_power_response_with_crc[PZEM_RESPONSE_ID_OFFSET])
    {
        case PZEM_READ_REGISTERS_COMMAND_ID:
            get_power_response();
            break;
        case PZEM_ERROR_RESPONSE_ID:
            get_error_response();
            break;
    }
}

static void get_power_response()
{
    if (io_read(get_power_response_with_crc + 2, 1, 100) == -1)
    {
        return;
    }

    if (get_power_response_with_crc[PZEM_RESPONSE_BYTE_COUNT_OFFSET] != sizeof(uint16_t) * PZEM_REGISTER_READ_COUNT)
    {
        return;
    }

    if (io_read(get_power_response_with_crc + 3, get_power_response_with_crc[PZEM_RESPONSE_BYTE_COUNT_OFFSET] + 2, 100) == -1)
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
        get_power_response_with_crc[PZEM_RESPONSE_VOLTAGE_LOW_OFFSET];

    uint32_t amp_reading =
        ((uint32_t) get_power_response_with_crc[PZEM_RESPONSE_AMPERAGE_HIGH_HIGH_OFFSET] << 24) |
        ((uint32_t) get_power_response_with_crc[PZEM_RESPONSE_AMPERAGE_HIGH_LOW_OFFSET]) << 16 |
        (get_power_response_with_crc[PZEM_RESPONSE_AMPERAGE_LOW_HIGH_OFFSET] << 8) |
        get_power_response_with_crc[PZEM_RESPONSE_AMPERAGE_LOW_LOW_OFFSET];

    bool current_draw_detected = (volt_reading > 1000 && volt_reading < 2500 && amp_reading > 3000);
    
    if (current_draw_detected != last_current_draw_detected)
    {
        last_current_draw_detected = current_draw_detected;

        if (current_draw_detected)
        {
            power_out_on();
        }
        else
        {
            power_out_off();
        }
    }
}

static void get_error_response()
{
    if (io_read(get_power_response_with_crc + 2, 3, 100) == -1)
    {
        return;
    }

    // Error information might be useful, but without
    // being debugged, there's no way to observe it.
}
