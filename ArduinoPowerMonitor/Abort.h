#pragma once

void abort_with_code(uint8_t code) __attribute__((__noreturn__));

#define abort(x) abort_with_code(x)
