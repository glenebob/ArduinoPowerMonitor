#include <avr/io.h>

#include "Bits.h"

void power_out_init()
{
    DDRD |= BIT2;
    PORTD &= ~BIT2;
}

void power_out_on()
{
    PORTD |= BIT2;
}

void power_out_off()
{
    PORTD &= ~BIT2;
}
