#include <stdint.h>

#include <avr/interrupt.h>

#include "Abort.h"

static uint8_t interruptLevel;

void interrupts_init()
{
    interruptLevel = 1;
}

void interrupts_enter_handler()
{
    if (interruptLevel)
    {
        fatal(10);
    }

    ++interruptLevel;
}

void interrupts_exit_handler()
{
    if (interruptLevel != 1)
    {
        fatal(11);
    }

    --interruptLevel;
}

void interrupts_raise_level()
{
    cli();
    
    ++interruptLevel;
}

void interrupts_release_level()
{
    if (!interruptLevel)
    {
        fatal(12);
    }
    
    --interruptLevel;
    
    if (!interruptLevel)
    {
        sei();
    }
}
