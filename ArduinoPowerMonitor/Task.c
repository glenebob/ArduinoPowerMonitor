#include <stdint.h>
#include <stdlib.h>

#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "Types.h"
#include "Error.h"
#include "Abort.h"
#include "Interrupt.h"
#include "Task.h"

typedef struct
{
    task_handler_t handler;
    void *arguments;
} task_t;

struct
{
    uint8_t last;
    uint8_t next_in;
    int8_t next_out;
    struct
    {
        task_handler_t handler;
        void *arguments;
    } tasks[5];
} task_queue;

void task_queue_init()
{
    task_queue.last = sizeof(task_queue.tasks) / sizeof(task_queue.tasks[0]) - 1;
    task_queue.next_in = 0;
    task_queue.next_out = -1;
}

void task_queue_push(task_handler_t handler, void *arguments)
{
    interrupt_raise_level();

    if (task_queue.next_in == task_queue.next_out)
    {
        fatal(ERR_TASK_TABLE_FULL);
    }

    task_queue.tasks[task_queue.next_in].handler = handler;
    task_queue.tasks[task_queue.next_in].arguments = arguments;

    if (task_queue.next_out == -1)
    {
        task_queue.next_out = task_queue.next_in;
    }

    if (task_queue.next_in == task_queue.last)
    {
        task_queue.next_in = 0;
    }
    else
    {
        ++task_queue.next_in;
    }

    interrupt_release_level();
}

bool task_queue_pop(task_handler_t *handler, void **arguments)
{
    interrupt_raise_level();

    if (task_queue.next_out == -1)
    {
        interrupt_release_level();

        return false;
    }

    *handler = task_queue.tasks[task_queue.next_out].handler;
    *arguments = task_queue.tasks[task_queue.next_out].arguments;

    ++task_queue.next_out;

    if (task_queue.next_out > task_queue.last)
    {
        task_queue.next_out = 0;
    }

    if (task_queue.next_out == task_queue.next_in)
    {
        task_queue.next_out = -1;
    }

    interrupt_release_level();

    return true;
}

void task_queue_run()
{
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();

    // Interrupt Level initializes to 1 because interrupts are off at boot.
    // This lowers it to zero and enables interrupts for the first time.
    interrupt_release_level();

    for (;;)
    {
        sleep_cpu();

        task_handler_t handler;
        void *arguments;

        if (task_queue_pop(&handler, &arguments))
        {
            handler(arguments);
        }
    }
}