#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

#include "types.h"

u16 rand_u16();

void print_err(const char *your_msg);

void print_bytes(byte *bytes, size_t len);

#endif