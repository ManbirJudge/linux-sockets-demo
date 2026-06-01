#include "my_string.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

String str_new(const char *txt) {
    String str = { .data = NULL, .len = 0 };
    if (!txt) return str;

    size_t len = strlen(txt);

    str.data = malloc(sizeof(char) * len + 1);
    if (!str.data) return str;

    memcpy(str.data, txt, len);
    str.data[len] = '\0';
    str.len = len;

    // printf("Created new string: %p %s\n", str.data, str.data);

    return str;
}

String str_from_bw(BufWriter *bw) {
    String str = { .data = NULL, .len = 0 };
    if (!bw) return str;

    if (bw->size == 0)
        bw_write_u8(bw, '\0');
    else if (bw->data[bw->size - 1] != '\0')
        bw_write_u8(bw, '\0');

    str.data = (char*)realloc(bw->data, bw->size);  // TODO: maybe reallocation is worth performance cost and potential leaks if it fails?
    str.len = bw->size - 1;

    bw->data = NULL;
    bw->cap = 0;
    bw->size = 0;

    return str;
}

String str_concat(const char* txt1, const char *txt2) {
    String str = { .data = NULL, .len = 0 };
    if (!(txt1 && txt2)) return str;

    size_t len1 = strlen(txt1);
    size_t len2 = strlen(txt2);

    str.data = malloc(sizeof(char) * (len1 + len2) + 1);
    if (!str.data) return str;

    memcpy(str.data, txt1, len1);
    memcpy(str.data + len1, txt2, len2);
    str.data[len1 + len2] = '\0';
    str.len = len1 + len2;

    return str;
}

void str_free(String *str) {
    if (str) {
        // printf("Freeing string: %p %s\n", str->data, str->data);
        if (str->data) free(str->data);
        str->data = NULL;
        str->len = 0;
    }
}