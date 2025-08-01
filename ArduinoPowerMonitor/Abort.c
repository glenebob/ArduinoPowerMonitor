#include <stdint.h>

#include <avr/interrupt.h>

#include "Led.h"

static const uint32_t bitDelays[] = { 200000, 800000 };

void abort_with_code(uint8_t code)
{
    cli();

    uint8_t significantBitCount = 2;
        
    for (int8_t shift = 7; shift >= 0; --shift)
    {
        if (code >> shift)
        {
            significantBitCount += shift;
            break;
        }
    }
    
    for (;;)
    {
        for (int8_t shift = significantBitCount - 1; shift >= 0; --shift)
        {
            uint32_t bitDelay = bitDelays[((code >> shift) & 0b00000001)];

            led_on();

            for (uint32_t i = 0; i < bitDelay; ++i);
            
            led_off();

            for (uint32_t i = 0; i < 200000; ++i);
        }

        for (uint32_t i = 0; i < 2000000; ++i);
    }
}
