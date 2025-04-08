#include <stdint.h>
#include <stdlib.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#include "Types.h"
#include "Abort.h"
#include "SoftwareTimer.h"
#include "ModBusCrc16.h"
#include "AsyncIo.h"
#include "PowerOut.h"

#include "PowerMonitor.h"

static uint8_t get_power_request_with_crc[] = { 0x01, 0x04, 0x00, 0x00, 0x00, 3, 0x00, 0x00 };
static uint8_t get_power_response[11];
static bool last_current_draw_detected;
static current_draw_change_handler_t current_draw_change_handler;

static void get_power_request_sent(void *completed);
static void get_power_response_header_received(void *completed);
static void get_power_response_length_received(void *completed);
static void get_power_response_data_received(void *completed);
static void get_power_response_error_received(void *completed);
static void read_all_bytes();
static void turn_on_power_timer_handler(void *arguments);
static void get_power_timer_handler(void *parameters);

void power_monitor_init(current_draw_change_handler_t change_handler)
{
    last_current_draw_detected = false;

    uint16_t crc = crc16(get_power_request_with_crc, sizeof(get_power_request_with_crc) - 2);

    get_power_request_with_crc[6] = crc;
    get_power_request_with_crc[7] = crc >> 8;
}

void power_monitor_begin(current_draw_change_handler_t change_handler)
{
    current_draw_change_handler = change_handler;

    timers_add_interrupts(turn_on_power_timer_handler, NULL, 1000, false);
}

bool is_current_draw_detected()
{
    return last_current_draw_detected;
}

void turn_on_power_timer_handler(void *arguments)
{
    power_out_on();
    timers_add_interrupts(get_power_timer_handler, NULL, 4000, false);
}

void get_power_timer_handler(void *arguments)
{
    io_write(get_power_request_with_crc, sizeof(get_power_request_with_crc), get_power_request_sent, 200);
}

static void get_power_request_sent(void *completed)
{
    if (!completed)
    {
        read_all_bytes();

        return;
    }

    io_read(get_power_response, 2, get_power_response_header_received, 200);
}

static void get_power_response_header_received(void *completed)
{
    if (!completed)
    {
        read_all_bytes();

        return;
    }

    if (get_power_response[0] != get_power_request_with_crc[0])
    {
        die();
    }

    if (get_power_response[1] == get_power_request_with_crc[1])
    {
        io_read(get_power_response + 2, 1, get_power_response_length_received, 200);
    }
    else if (get_power_response[1] == 0x8A)
    {
        io_read(get_power_response + 2, 3, get_power_response_error_received, 200);
    }
    else
    {
        die();
    }
}

static void get_power_response_length_received(void *completed)
{
    if (!completed)
    {
        read_all_bytes();

        return;
    }

    if (get_power_response[2] == 6)
    {
        io_read(get_power_response + 3, get_power_response[2] + 2, get_power_response_data_received, 200);
    }
    else
    {
        die();
    }
}

static void get_power_response_data_received(void *completed)
{
    if (!completed)
    {
        read_all_bytes();

        return;
    }

    uint16_t expected_crc = crc16(get_power_response, 9);

    uint16_t actual_crc =
        get_power_response[9] |
        (get_power_response[10] << 8);

    if (actual_crc != expected_crc)
    {
        return;
    }

    uint16_t volt_reading =
        (get_power_response[3] << 8) |
        (get_power_response[4] << 0);

    uint32_t amp_reading =
        (get_power_response[5] << 8) |
        (get_power_response[6] << 0) |
        (((uint32_t) get_power_response[7]) << 24) |
        (((uint32_t) get_power_response[8]) << 16);

    bool current_draw_detected = (volt_reading > 1000 && volt_reading < 2500 && amp_reading > 3000);
    
    if (current_draw_detected != last_current_draw_detected)
    {
         last_current_draw_detected = current_draw_detected;
         current_draw_change_handler(current_draw_detected);
    }

    timers_add_interrupts(get_power_timer_handler, NULL, 1000, false);
}

static void get_power_response_error_received(void *completed)
{
    if (!completed)
    {
        read_all_bytes();

        return;
    }

    // Error information might be useful, but without
    // being debugged, there's no way to observe it.
}

static void read_one_byte_handler(void *completed)
{
    if (!completed)
    {
        timers_add_interrupts(get_power_timer_handler, NULL, 5000, false);

        return;
    }

    io_read(get_power_response, 1, read_one_byte_handler, 200);
}

static void read_all_bytes()
{
    io_read(get_power_response, 1, read_one_byte_handler, 200);
}