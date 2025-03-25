#pragma once

#include "Types.h"

void timers_init();
uint8_t timers_add(task_handler_t handler, void *arguments, uint16_t ellapsed, bool recurring);
void timers_cancel(uint8_t timer_id);
void timers_process_active();

