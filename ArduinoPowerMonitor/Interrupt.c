#include <stdint.h>

#include <avr/interrupt.h>

#include "Error.h"
#include "Abort.h"

static uint8_t interruptLevel;

void interrupt_init()
{
    interruptLevel = 1;
}

void interrupt_enter_handler()
{
    if (interruptLevel)
    {
        fatal(ERR_INT_ENTER_BAD_LEVEL);
    }

    ++interruptLevel;
}

void interrupt_exit_handler()
{
    if (interruptLevel != 1)
    {
        fatal(ERR_INT_EXIT_BAD_LEVEL);
    }

    --interruptLevel;
}

void interrupt_raise_level()
{
    cli();

    if (interruptLevel == 0xFF)
    {
        fatal(ERR_INT_RAISE_BAD_LEVEL);
    }
    
    ++interruptLevel;
}

void interrupt_release_level()
{
    if (!interruptLevel)
    {
        fatal(ERR_INT_RELEASE_BAD_LEVEL);
    }
    
    --interruptLevel;
    
    if (!interruptLevel)
    {
        sei();
    }
}
