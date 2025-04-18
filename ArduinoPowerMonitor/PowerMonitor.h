#pragma once

#include "Types.h"

typedef void (*current_draw_change_handler_t)(bool);

void power_monitor_init();
void power_monitor_begin(current_draw_change_handler_t change_handler);
bool is_current_draw_detected();
