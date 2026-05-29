#include "my_string.h"

#include <stdlib.h>
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

void str_free(String *str) {
    if (str) {
        if (str->data) free(str->data);
        str->data = NULL;
        str->len = 0;
    }
}