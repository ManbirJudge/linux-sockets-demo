#ifndef BUFFER_WRITER_H
#define BUFFER_WRITER_H

#include <stdbool.h>
#include <stddef.h>

#include "types.h"
#include "endianess.h"

typedef struct {
    byte *data;
    size_t size;
    size_t cap; 
} BufWriter;

BufWriter bw_init();

bool bw_write(BufWriter *bw, const void *src, size_t size);
bool bw_write_u8(BufWriter *bw, u8 v);
bool bw_write_u16(BufWriter *bw, u16 v);
bool bw_write_u32(BufWriter *bw, u32 v);
bool bw_write_u64(BufWriter *bw, u64 v);

// CRITICAL: in following aliases which use swappers, v is expanded twice which can be problematic

#ifdef IS_LITLE_ENDIAN
    #define bw_write_u16_le(bw, v) bw_write_u16(bw, v)
    #define bw_write_u32_le(bw, v) bw_write_u32(bw, v)
    #define bw_write_u64_le(bw, v) bw_write_u64(bw, v)

    #define bw_write_u16_be(bw, v) bw_write_u16(bw, swap_16(v))
    #define bw_write_u32_be(bw, v) bw_write_u32(bw, swap_32(v))
    #define bw_write_u64_be(bw, v) bw_write_u64(bw, swap_64(v))
#else
    #define bw_write_u16_be(bw, v) bw_write_u16(bw, v)
    #define bw_write_u32_be(bw, v) bw_write_u32(bw, v)
    #define bw_write_u64_be(bw, v) bw_write_u64(bw, v)

    #define bw_write_u16_le(bw, v) bw_write_u16(bw, swap_16(v))
    #define bw_write_u32_le(bw, v) bw_write_u32(bw, swap_32(v))
    #define bw_write_u64_le(bw, v) bw_write_u64(bw, swap_64(v))
#endif

void bw_free(BufWriter *bw);

#endif