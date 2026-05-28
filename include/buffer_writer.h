#ifndef BUFFER_WRITER_H
#define BUFFER_WRITER_H

#include <stdbool.h>
#include <stddef.h>

#include "types.h"

#if defined(__linux__) || defined(__CYGWIN__)
    #include <endian.h>
#elif defined(__Apple__)
    #include <machine/endian.h>
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    #include <sys/endian.h>
#endif

#if defined(__GNUC__) || defined (__clang__)
    #define swap_16(x) __builtin_bswap16(x)
    #define swap_32(x) __builtin_bswap32(x)
    #define swap_64(x) __builtin_bswap64(x)
#elif defined(_MSC_VER)
    #define swap_16(x) _byteswap_ushort(x)
    #define swap_32(x) _byteswap_ulong(x)
    #define swap_64(x) _byteswap_uint64(x)
#else
    #define swap_16(x) ((u16)(((u16)(x) >> 8) | ((u16)(x) << 8)))
    #define swap_32(x) ((u32)((((u32)(x) & 0xff000000) >> 24) | (((u32)(x) & 0x00ff0000) >> 8) | (((u32)(x) & 0x0000ff00) << 8)  | (((u32)(x) & 0x000000ff) << 24)))
    #define swap_64(x) ((u64)((((u64)(x) & 0xff00000000000000ULL) >> 56) | (((u64)(x) & 0x00ff000000000000ULL) >> 40) | (((u64)(x) & 0x0000ff0000000000ULL) >> 24) | (((u64)(x) & 0x000000ff00000000ULL) >> 8)  | (((u64)(x) & 0x00000000ff000000ULL) << 8)  | (((u64)(x) & 0x0000000000ff0000ULL) << 24) | (((u64)(x) & 0x000000000000ff00ULL) << 40) | (((u64)(x) & 0x00000000000000ffULL) << 56)))
#endif

typedef struct {
    byte *data;
    size_t size;
    size_t cap; 
} BufWriter;

BufWriter bw_init();

bool bw_write(BufWriter *bw, const void *src, size_t n);
bool bw_write_u8(BufWriter *bw, u8 v);
bool bw_write_u16(BufWriter *bw, u16 v);
bool bw_write_u32(BufWriter *bw, u32 v);
bool bw_write_u64(BufWriter *bw, u64 v);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define bw_write_u16_le(bw, v) bw_write_u16(bw, v)
    #define bw_write_u32_le(bw, v) bw_write_u32(bw, v)
    #define bw_write_u64_le(bw, v) bw_write_u64(bw, v)

    #define bw_write_u16_be(bw, v) bw_write_u16(bw, swap_16(v))
    #define bw_write_u32_be(bw, v) bw_write_u32(bw, swap_32(v))
    #define bw_write_u64_be(bw, v) bw_write_u64(bw, swap_64(v))
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define bw_write_u16_be(bw, v) bw_write_u16(bw, v)
    #define bw_write_u32_be(bw, v) bw_write_u32(bw, v)
    #define bw_write_u64_be(bw, v) bw_write_u64(bw, v)

    #define bw_write_u16_le(bw, v) bw_write_u16(bw, swap_16(v))
    #define bw_write_u32_le(bw, v) bw_write_u32(bw, swap_32(v))
    #define bw_write_u64_le(bw, v) bw_write_u64(bw, swap_64(v))
#else
    #error "Unknown archictecture endianness! You are on a weird-ass system."
#endif

void bw_free(BufWriter *bw);

#endif