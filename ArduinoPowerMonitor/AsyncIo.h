#pragma once

#include "Task.h"

void io_task_init();
void io_read(uint8_t *buffer, uint8_t buffer_length, task_handler_t handler, void *arguments, uint16_t timeout);
void io_write(uint8_t *buffer, uint8_t buffer_length, task_handler_t handler, void *arguments, uint16_t timeout);
