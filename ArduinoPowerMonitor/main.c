#include <stdint.h>
#include <stdlib.h>

#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "Types.h"
#include "ModBusCrc16.h"
#include "Task.h"
#include "SoftwareTimer.h"
#include "AsyncIo.h"
#include "PowerMonitor.h"
#include "Led.h"

static int led_count;

static void init();
static void run();
static void timer_20_ms_handler(void *arguments);
static void timer_1_s_handler(void *arguments);

int main()
{
    init();
    run();

    return 0;
}

static void init()
{
    task_queue_init();
    timers_init();
    io_task_init();
    power_monitor_init();

    led_count = 0;

    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();

    timers_add(timer_20_ms_handler, NULL, 20, true);
    timers_add(timer_1_s_handler, NULL, 1000, true);

    sei();
}

static void run()
{
    for (;;)
    {
        sleep_cpu();

        cli();

        task_handler_t handler;
        void *arguments;
        bool have_task = task_queue_pop(&handler, &arguments);

        sei();

        if (have_task)
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

static void timer_1_s_handler(void *arguments)
{
    begin_get_power(current_draw_change);
}

