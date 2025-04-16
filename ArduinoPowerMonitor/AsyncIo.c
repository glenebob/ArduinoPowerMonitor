#include <stdint.h>
#include <stdlib.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#include "Types.h"
#include "Abort.h"
#include "Task.h"
#include "SoftwareTimer.h"

#define BAUD 9600
#define BAUDRATE ((F_CPU) / (BAUD * 16UL) -1)

typedef struct
{
    bool active;
    uint8_t *buffer;
    uint8_t buffer_length;
    uint8_t offset;
    task_handler_t handler;
    int8_t timer_id;
} io_task_t;

static io_task_t read_task;
static io_task_t write_task;

void io_task_init()
{
    read_task.active = false;
    write_task.active = false;

    UBRR0H = (uint8_t) (BAUDRATE >> 8);
    UBRR0L = (uint8_t) BAUDRATE;
    // Enable receiver and transmitter
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    // Set frame format: 8data, 2stop bit
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXCIE0) | (1 << TXEN0);
};

static void io_read_timeout(void *argument)
{
    cli();

    read_task.active = false;

    task_queue_push(read_task.handler, false);

    sei();
}

void io_read(uint8_t *buffer, uint8_t buffer_length, task_handler_t handler, uint16_t timeout)
{
    cli();

    if (read_task.active)
    {
        die();
    }

    read_task.active = true;
    read_task.buffer = buffer;
    read_task.buffer_length = buffer_length;
    read_task.offset = 0;
    read_task.handler = handler;

    if (UCSR0A & (1 << RXC0))
    {
        read_task.buffer[read_task.offset++] = UDR0;
        
        if (read_task.offset == read_task.buffer_length)
        {
            read_task.active = false;
            
            task_queue_push(read_task.handler, (void*) true);
        }
    }

    if (read_task.active && timeout > 0)
    {
        read_task.timer_id = timers_add(io_read_timeout, NULL, timeout, false);
    }
    else
    {
        read_task.timer_id = -1;
    }

    sei();
}

static void io_write_timeout(void *argument)
{
    cli();

    write_task.active = false;

    task_queue_push(write_task.handler, false);
    
    sei();
}

void io_write(uint8_t *buffer, uint8_t buffer_length, task_handler_t handler, uint16_t timeout)
{
    cli();

    if (write_task.active)
    {
        die();
    }

    write_task.active = true;
    write_task.buffer = buffer;
    write_task.buffer_length = buffer_length;
    write_task.offset = 0;
    write_task.handler = handler;

    if (UCSR0A & (1 << UDRE0))
    {
        UDR0 = write_task.buffer[write_task.offset++];
        
        if (write_task.offset == write_task.buffer_length)
        {
            write_task.active = false;
            
            task_queue_push(write_task.handler, (void*) true);
        }
    }

    if (write_task.active && timeout >= 0)
    {
        write_task.timer_id = timers_add(io_write_timeout, NULL, timeout, false);
    }
    else
    {
        write_task.timer_id = -1;
    }
    
    sei();
}

ISR(USART_RX_vect)
{
    if (read_task.active)
    {
        read_task.buffer[read_task.offset++] = UDR0;

        if (read_task.offset == read_task.buffer_length)
        {
            read_task.active = false;

            if (read_task.timer_id >= 0)
            {
                timers_cancel(read_task.timer_id);
            }

            task_queue_push(read_task.handler, (void*) true);
        }
    }
    else
    {
        // Byte dropped/lost.
    }
}

ISR(USART_TX_vect)
{
    if (write_task.active)
    {
        UDR0 = write_task.buffer[write_task.offset++];
        
        if (write_task.offset == write_task.buffer_length)
        {
            write_task.active = false;

            if (write_task.timer_id >= 0)
            {
                timers_cancel(write_task.timer_id);
            }

            task_queue_push(write_task.handler, (void*) true);
        }
    }
}
