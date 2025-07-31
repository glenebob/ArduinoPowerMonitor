#include <stdint.h>

#include <avr/interrupt.h>

#include "Led.h"

void fatal(uint8_t code)
{
    cli();
    
    for (;;)
    {
        for (int8_t shift = 7; shift >= 0; --shift)
        {
            uint32_t bitDelay;

            if ((code >> shift) & 0x00000001)
            {
                bitDelay = 800000;
            }
            else
            {
                bitDelay = 300000;
            }

            led_on();

            for (uint32_t i = 0; i < bitDelay; ++i);
            
            led_off();

            for (uint32_t i = 0; i < 200000; ++i);
        }

        for (uint32_t i = 0; i < 2000000; ++i);
    }
}
