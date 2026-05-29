#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

#include "types.h"

u16 rand_u16();

void print_err(const char *your_msg);

// Source - https://stackoverflow.com/a/3208376
// Posted by William Whyte, modified by community. See post 'Timeline' for change history
// Retrieved 2026-05-29, License - CC BY-SA 4.0
#define BYTE_TO_BIN_PAT "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BIN(byte)      \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0') 

void print_bytes(const  byte *bytes, size_t len);

#endif