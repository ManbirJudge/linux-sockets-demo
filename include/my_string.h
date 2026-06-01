#ifndef MY_STRING_H
#define MY_STRING_H

#include <stddef.h>

#include "buffer_writer.h"

typedef struct {
    char *data;
    size_t len;
} String;

String str_new(const char* txt);
String str_from_bw(BufWriter *bw);
String str_concat(const char* txt1, const char *txt2);

void str_free(String *str);

#endif