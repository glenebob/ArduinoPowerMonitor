#include <avr/io.h>

void led_init()
{
    DDRB |= 1 << DDB5;
    PORTB &= ~(1 << PORTB5);
}

void led_on()
{
    PORTB |= 1 << PORTB5;
}

void led_off()
{
    PORTB &= ~(1 << PORTB5);
}
