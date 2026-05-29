#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

u16 rand_u16() {
    return (u16)(rand() & 0xFFFF);
}

void print_err(const char *your_msg) {
    int code = errno;
    printf("Error: %s\n  (%d) %s\n", your_msg, code, strerror(code));
}

void print_bytes(const byte *bytes, size_t len) {
    for (size_t i = 0; i < len; i++) printf("%02x ", bytes[i]);
    printf("\n");
}