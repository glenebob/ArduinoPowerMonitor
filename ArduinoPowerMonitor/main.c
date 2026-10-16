#include <stdint.h>
#include <string.h>

#include <avr/pgmspace.h>

#include "Application.h"
#include "Types.h"
#include "Thread.h"
#include "Context.h"
#include "Bits.h"

#ifdef TEST
extern uint8_t _end;
#endif

__attribute__((OS_main))
int main(void)
{
#ifdef TEST
    for (uint8_t *address = &_end; address <= (uint8_t*) 0x08FD; ++address)
    {
        *address = 0;
    }

    static const __flash char string_main[] PROGMEM = "Main!";

    char status[sizeof(string_main) - 1];

    memcpy_P(status, string_main, sizeof(status));
#endif

    DDRB |= BIT5;
    PORTB &= ~BIT5;

    threads_initialize();

    JUMP(thread_kernel_entry);
}












/*
static void current_draw_change(bool current_draw_detected);

int main()
{
    led_init();
    power_out_init();
    interrupt_init();
    task_queue_init();
    timer_init();
    io_task_init();
    power_monitor_init();
    power_monitor_begin(current_draw_change);

    task_queue_run(); // Never returns
}

static void current_draw_change(bool current_draw_detected)
{
    if (current_draw_detected)
    {
        power_out_off();
    }
    else
    {
        power_out_on();
    }
}
*/