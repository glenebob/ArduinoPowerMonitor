#include <avr/io.h>

#include "Bits.h"

void led_init()
{
    DDRB |= BIT5;
    PORTB &= ~BIT5;
}

void led_on()
{
    PORTB |= BIT5;
}

void led_off()
{   
    PORTB &= ~BIT5;
}
