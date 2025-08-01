#pragma once

void interrupt_init();
void interrupt_boot();
void interrupt_enter_handler();
void interrupt_exit_handler();
void interrupt_raise_level();
void interrupt_release_level();
