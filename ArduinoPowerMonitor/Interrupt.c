#include <stdint.h>
#include <stdlib.h>

#include <avr/interrupt.h>

#include "Error.h"

static uint8_t interruptLevel;

void interrupt_init()
{
    interruptLevel = 1;
}

void interrupt_boot()
{
    if (interruptLevel != 1)
    {
        exit(ERR_INT_BOOT_BAD_LEVEL);
    }

    --interruptLevel;
    sei();
}

void interrupt_enter_handler()
{
    if (interruptLevel)
    {
        exit(ERR_INT_ENTER_BAD_LEVEL);
    }

    ++interruptLevel;
}

void interrupt_exit_handler()
{
    if (interruptLevel != 1)
    {
        exit(ERR_INT_EXIT_BAD_LEVEL);
    }

    --interruptLevel;
}

void interrupt_raise_level()
{
    cli();

    if (interruptLevel == 0xFF)
    {
        exit(ERR_INT_RAISE_BAD_LEVEL);
    }
    
    ++interruptLevel;
}

void interrupt_release_level()
{
    if (!interruptLevel)
    {
        exit(ERR_INT_RELEASE_BAD_LEVEL);
    }
    
    --interruptLevel;
    
    if (!interruptLevel)
    {
        sei();
    }
}
