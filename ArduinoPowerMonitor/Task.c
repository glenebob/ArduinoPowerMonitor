#include <stdint.h>
#include <stdlib.h>

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
    } tasks[20];
} task_queue;

void task_queue_init()
{
    task_queue.last = sizeof(task_queue.tasks) / sizeof(task_queue.tasks[0]) - 1;
    task_queue.next_in = 0;
    task_queue.next_out = -1;
}

void task_queue_push(task_handler_t handler, void *arguments)
{
    if (task_queue.next_in == task_queue.next_out)
    {
        abort();
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
}

int task_queue_pop(task_handler_t *handler, void **arguments)
{
    if (task_queue.next_out == -1)
    {
        return 0;
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
    
    return 1;
}
