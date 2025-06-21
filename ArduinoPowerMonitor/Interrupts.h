#pragma once

void interrupts_init();
void interrupts_enter_handler();
void interrupts_exit_handler();
void interrupts_raise_level();
void interrupts_release_level();
