#include <avr/io.h>

void power_out_init()
{
    DDRD |= 1 << DDD2;
    PORTD &= ~(1 << PORTD2);
}

void power_out_on()
{
    PORTD |= 1 << PORTD2;
}

typedef void (*reboot)();

void power_out_off()
{
    PORTD &= ~(1 << PORTD2);
}
