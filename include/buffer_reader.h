#ifndef BUFFER_READER_H
#define BUFFER_READER_H

#include <stdbool.h>
#include <stddef.h>

#include "types.h"
#include "endianess.h"

typedef struct {
    const byte *data;
    size_t size;
    size_t pos;
} BufReader;

BufReader br_init(const void *data, size_t size);

bool br_read(BufReader *br, void *dst, size_t size);

bool br_read_u8(BufReader *br, u8 *out);
bool br_read_u16(BufReader *br, u16 *out);
bool br_read_u32(BufReader *br, u32 *out);
bool br_read_u64(BufReader *br, u64 *out);
bool br_read_u16_inv(BufReader *br, u16 *out);
bool br_read_u32_inv(BufReader *br, u32 *out);
bool br_read_u64_inv(BufReader *br, u64 *out);

bool br_read_bytes(BufReader *br, byte *dst, size_t n);  // can be used for known-length strings

#ifdef IS_LITLE_ENDIAN
    #define br_read_u16_le(br, out) br_read_u16(br, out)
    #define br_read_u32_le(br, out) br_read_u32(br, out)
    #define br_read_u64_le(br, out) br_read_u64(br, out)

    #define br_read_u16_be(br, out) br_read_u16_inv(br, out)
    #define br_read_u32_be(br, out) br_read_u32_inv(br, out)
    #define br_read_u64_be(br, out) br_read_u64_inv(br, out)
#else
    #define br_read_u16_le(br, out) br_read_u16_inv(br, out)
    #define br_read_u32_le(br, out) br_read_u32_inv(br, out)
    #define br_read_u64_le(br, out) br_read_u64_inv(br, out)
    
    #define br_read_u16_be(br, out) br_read_u16(br, out)
    #define br_read_u32_be(br, out) br_read_u32(br, out)
    #define br_read_u64_be(br, out) br_read_u64(br, out)
#endif

#endif