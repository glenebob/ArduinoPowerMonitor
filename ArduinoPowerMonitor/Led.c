#include <avr/io.h>

#include "led.h"

void led_init()
{
    DDRB |= 1 << PINB5; // Port D pin 5 is output (LED)
}

void led_on()
{
    PORTB |= 1 << PINB5;
}

void led_off()
{
    PORTB &= ~(1 << PINB5);
}
