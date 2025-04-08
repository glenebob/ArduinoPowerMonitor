#include <stdint.h>
#include <stdlib.h>

#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "Types.h"
#include "ModBusCrc16.h"
#include "Task.h"
#include "SoftwareTimer.h"
#include "AsyncIo.h"
#include "PowerOut.h"
#include "PowerMonitor.h"
#include "Led.h"

static int led_count;

static void init();
static void run();
static void timer_20_ms_handler(void *arguments);
static void current_draw_change(bool current_draw_detected);

int main()
{
    init();
    run();

    return 0;
}

static void init()
{
    led_init();
    task_queue_init();
    timers_init();
    io_task_init();
    power_out_init();
    power_monitor_init();

    led_count = 0;

    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();

    timers_add(timer_20_ms_handler, NULL, 20, true);
    power_monitor_begin(current_draw_change);
}

static void run()
{
    sei();

    for (;;)
    {
        sleep_cpu();

        task_handler_t handler;
        void *arguments;

        if (task_queue_pop_interrupts(&handler, &arguments))
        {
            handler(arguments);
        }
    }
}

static void timer_20_ms_handler(void *arguments)
{
    if (is_current_draw_detected())
    {
        switch (led_count)
        {
            case 0:
                led_on();
                led_count++;
                break;
            case 3:
                led_off();
                led_count++;
                break;
            case 6:
                led_count = 0;
                break;
            default:
                led_count++;
                break;
        }
    }
    else
    {
        switch (led_count)
        {
            case 0:
                led_on();
                led_count++;
                break;
            case 10:
                led_off();
                led_count++;
                break;
            case 20:
                led_count = 0;
                break;
            default:
                led_count++;
                break;
        }
    }
}

static void current_draw_change(bool current_draw_detected)
{
/*
    if (current_draw_detected)    
    {
        led_on();
    }
    else
    {
        led_off();
    }
*/

    led_count = 0;
}


