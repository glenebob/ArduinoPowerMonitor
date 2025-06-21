#include <stdint.h>

#include <avr/interrupt.h>

#include "Led.h"

void fatal(uint8_t code)
{
    cli();
    
    for (;;)
    {
        for (uint8_t j = 0; j < code; ++j)
        {
            led_on();

            for (uint32_t i = 0; i < 200000; ++i);
        
            led_off();

            for (uint32_t i = 0; i < 200000; ++i);
        }            

        for (uint32_t i = 0; i < 800000; ++i);
    }
}
