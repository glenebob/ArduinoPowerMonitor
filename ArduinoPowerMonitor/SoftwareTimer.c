#include <stdint.h>
#include <stdlib.h>

#include <avr/interrupt.h>

#include "Types.h"
#include "Error.h"
#include "Interrupt.h"
#include "Task.h"
#include "Bits.h"

typedef struct
{
    task_handler_t handler;
    void *arguments;
    uint16_t elapsed;
    uint16_t trigger;
    bool recurring;
    int8_t next;
} timer_t;

struct
{
    uint8_t last;
    int8_t first_free;
    int8_t first_active;
    timer_t timers[5];
} timer_table;

void timer_init()
{
    timer_table.last = sizeof(timer_table.timers) / sizeof(timer_table.timers[0]) - 1;
    timer_table.first_free = 0;
    timer_table.first_active = -1;
    
    for (uint8_t i = 0; i < timer_table.last; ++i)
    {
        timer_table.timers[i].next = i + 1;
    }

    timer_table.timers[timer_table.last].next = -1;

    OCR0A = 250; // Match value, with prescaler 64, the timer will tick every 1 ms.
    //	TCNT0 = 0;
    //  TCCR0A |= 1 << COM0A1;
    TCCR0A |= BIT0; // Wave Generation Mode = PWM, phase correct
    TCCR0A |= BIT1; // Wave Generation Mode = CTC
    TCCR0B |= BIT3; // Wave Generation Mode = PWM, phase correct
    TCCR0B |= BIT0 | BIT1; // Prescaler bit 0 & 1 = prescaler 64
    //  TIMSK0 |= 1 << OCIE0A; // Interrupt on Match OCR0A
    TIMSK0 |= BIT0; // Interrupt on Overflow
}

uint8_t timer_add(task_handler_t handler, void *arguments, uint16_t ellapsed, bool recurring)
{
    interrupt_raise_level();
    
    if (timer_table.first_free == -1)
    {
        exit(ERR_TIMER_TABLE_FULL);
    }
    
    uint8_t timer_index = timer_table.first_free;
    timer_t *new_timer = &timer_table.timers[timer_index];

    timer_table.first_free = new_timer->next;
    
    if (timer_table.first_active >= 0)
    {
        new_timer->next = timer_table.first_active;
    }
    else
    {
        new_timer->next = -1;
    }

    timer_table.first_active = timer_index;
    
    new_timer->handler = handler;
    new_timer->arguments = arguments;
    new_timer->trigger = ellapsed;
    new_timer->elapsed = 0;
    new_timer->recurring = recurring;

    interrupt_release_level();
    
    return timer_index;
}

void timer_cancel(uint8_t timer_id)
{
    interrupt_raise_level();

    int8_t last_timer_index = -1;
    timer_t *last_timer = NULL;

    for (int8_t timer_index = timer_table.first_active; timer_index >= 0; )
    {
        timer_t *timer = &timer_table.timers[timer_index];

        if (timer_index == timer_id)
        {
            if (last_timer_index >= 0)
            {
                last_timer->next = timer->next;
            }
            else
            {
                timer_table.first_active = timer->next;
            }

            timer->next = timer_table.first_free;
            timer_table.first_free = timer_index;

            goto end;
        }
        else
        {
            last_timer_index = timer_index;
            last_timer = timer;
            timer_index = timer->next;
        }
    }

end:
    interrupt_release_level();
}

ISR(TIMER0_OVF_vect)
{
    interrupt_enter_handler();

    int8_t last_timer_index = -1;
    timer_t *last_timer = NULL;

    for (int8_t timer_index = timer_table.first_active; timer_index >= 0; )
    {
        timer_t *timer = &timer_table.timers[timer_index];

        ++timer->elapsed;
        
        if (timer->elapsed == timer->trigger)
        {
            if (!timer->recurring)
            {
                int8_t next_timer_index = timer->next;

                if (last_timer_index >= 0)
                {
                    last_timer->next = timer->next;

                    last_timer_index = -1;
                    last_timer = NULL;
                }
                else
                {
                    timer_table.first_active = timer->next;
                }

                timer->next = timer_table.first_free;
                timer_table.first_free = timer_index;

                timer_index = next_timer_index;
            }
            else
            {
                timer->elapsed = 0;
            }

            task_queue_push(timer->handler, timer->arguments);
        }
        else
        {
            last_timer_index = timer_index;
            last_timer = timer;
            timer_index = timer->next;
        }
    }

    interrupt_exit_handler();
}
