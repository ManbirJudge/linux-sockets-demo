#include "buffer_writer.h"

#include <stdlib.h>
#include <string.h>

BufWriter bw_init() {
    return (BufWriter){
        .data = malloc(64),
        .size = 0,
        .cap = 64
    };
}

bool bw_write(BufWriter *bw, const void *src, size_t size) {
    if (!bw->data) return false;

    if (size > bw->cap - bw->size) {
        size_t new_cap = bw->size + size;
        new_cap--;
        new_cap |= new_cap >> 1;
        new_cap |= new_cap >> 2;
        new_cap |= new_cap >> 4;
        new_cap |= new_cap >> 8;
        new_cap |= new_cap >> 16;
        #if __SIZEOF_POINTER__ == 8
        new_cap |= new_cap >> 32;
        #endif
        new_cap++;

        bw->cap = new_cap;
        bw->data = realloc(bw->data, new_cap);
        if (!bw->data) return false;
    }

    memcpy(bw->data + bw->size, src, size);
    bw->size += size;

    return true;
}

bool bw_write_u8(BufWriter *bw, uint8_t v) {
    return bw_write(bw, &v, sizeof(u8));
}
bool bw_write_u16(BufWriter *bw, u16 v) {
    return bw_write(bw, &v, sizeof(u16));
}
bool bw_write_u32(BufWriter *bw, u32 v) {
    return bw_write(bw, &v, sizeof(u32));
}
bool bw_write_u64(BufWriter *bw, u64 v) {
    return bw_write(bw, &v, sizeof(u64));
}

void bw_free(BufWriter *bw) {
    if (bw->data) free(bw->data);
    bw->size = 0;
    bw->cap = 0;
}