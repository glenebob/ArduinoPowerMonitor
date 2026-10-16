#include <avr/io.h>

#include "Application.h"
#include "Bits.h"

void power_out_initialize()
{
#ifdef TEST
    DDRB |= BIT5;
    PORTB &= ~BIT5;
#else
    DDRD |= BIT2;
    PORTD &= ~BIT2;
#endif
}

void power_out_on()
{
#ifdef TEST
    PORTB |= BIT5;
#else
    PORTD |= BIT2;
#endif
}

void power_out_off()
{
#ifdef TEST
    PORTB &= ~BIT5;
#else
    PORTD &= ~BIT2;
#endif
}
