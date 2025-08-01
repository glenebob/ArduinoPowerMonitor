#pragma once

#include <stdint.h>

#include "Types.h"
#include "Task.h"

void timer_init();
uint8_t timer_add(task_handler_t handler, void *arguments, uint16_t ellapsed, bool recurring);
void timer_cancel(uint8_t timer_id);
