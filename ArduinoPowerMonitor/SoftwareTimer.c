#include <stdint.h>
#include <stdlib.h>

#include <avr/interrupt.h>

#include "Types.h"
#include "Abort.h"
#include "Task.h"

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
} timers;

void timers_init()
{
    timers.last = sizeof(timers.timers) / sizeof(timers.timers[0]) - 1;
    timers.first_free = 0;
    timers.first_active = -1;
    
    for (uint8_t i = 0; i < timers.last; ++i)
    {
        timers.timers[i].next = i + 1;
    }

    timers.timers[timers.last].next = -1;

    OCR0A = 250; // Match value, with prescaler 64, the timer will tick every 1 ms.
    //	TCNT0 = 0;
    //  TCCR0A |= 1 << COM0A1;
    TCCR0A |= 1 << WGM00; // Wave Generation Mode = PWM, phase correct
    TCCR0A |= 1 << WGM01; // Wave Generation Mode = CTC
    TCCR0B |= 1 << WGM02; // Wave Generation Mode = PWM, phase correct
    TCCR0B |= (1 << CS00) | (1 << CS01);   // Prescaler bit 0 & 1 = prescaler 64
    //  TIMSK0 |= 1 << OCIE0A; // Interrupt on Match OCR0A
    TIMSK0 |= 1 << TOIE0; // Interrupt on Overflow
}

uint8_t timers_add(task_handler_t handler, void *arguments, uint16_t ellapsed, bool recurring)
{
    if (timers.first_free == -1)
    {
        die();
    }
    
    uint8_t timer_index = timers.first_free;
    timer_t *new_timer = &timers.timers[timer_index];

    timers.first_free = new_timer->next;
    
    if (timers.first_active >= 0)
    {
        new_timer->next = timers.first_active;
    }
    else
    {
        new_timer->next = -1;
    }

    timers.first_active = timer_index;
    
    new_timer->handler = handler;
    new_timer->arguments = arguments;
    new_timer->trigger = ellapsed;
    new_timer->elapsed = 0;
    new_timer->recurring = recurring;
    
    return timer_index;
}

uint8_t timers_add_interrupts(task_handler_t handler, void *arguments, uint16_t ellapsed, bool recurring)
{
    cli();
    uint8_t result = timers_add(handler, arguments, ellapsed, recurring);
    sei();

    return result;
}

void timers_cancel(uint8_t timer_id)
{
    int8_t last_timer_index = -1;
    timer_t *last_timer = NULL;

    for (int8_t timer_index = timers.first_active; timer_index >= 0; )
    {
        timer_t *timer = &timers.timers[timer_index];

        if (timer_index == timer_id)
        {
            if (last_timer_index >= 0)
            {
                last_timer->next = timer->next;
            }
            else
            {
                timers.first_active = timer->next;
            }

            timer->next = timers.first_free;
            timers.first_free = timer_index;

            return;
        }
        else
        {
            last_timer_index = timer_index;
            last_timer = timer;
            timer_index = timer->next;
        }
    }
}

void timers_cancel_interrupts(uint8_t timer_id)
{
    cli();
    timers_cancel(timer_id);
    sei();
}

ISR(TIMER0_OVF_vect)
{
    int8_t last_timer_index = -1;
    timer_t *last_timer = NULL;

    for (int8_t timer_index = timers.first_active; timer_index >= 0; )
    {
        timer_t *timer = &timers.timers[timer_index];

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
                    timers.first_active = timer->next;
                }

                timer->next = timers.first_free;
                timers.first_free = timer_index;

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
}
