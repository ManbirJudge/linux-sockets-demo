#include "buffer_reader.h"
#include "endianess.h"

#include <string.h>

BufReader br_init(const void *data, size_t size) {
    return (BufReader){
        .data = (const byte*)data,
        .size = size,
        .pos = 0
    };
}

bool br_read(BufReader *br, void *dst, size_t size) {
    if (size > br->size - br->pos) return false;
    
    memcpy(dst, br->data + br->pos, size);
    br->pos += size;

    return true;
}

bool br_read_u8(BufReader *br, u8 *out) {
    if (br->pos + 1 > br->size) return false;
    *out = br->data[br->pos++];
    return true;
}
bool br_read_u16(BufReader *br, u16 *out) {
    if (br->pos + 2 > br->size) return false;
    memcpy(out, br->data + br->pos, 2); 
    br->pos += 2;
    return true;
}
bool br_read_u32(BufReader *br, u32 *out) {
    if (br->pos + 4 > br->size) return false;
    memcpy(out, br->data + br->pos, 4); 
    br->pos += 4;
    return true;
}
bool br_read_u64(BufReader *br, u64 *out) {
    if (br->pos + 8 > br->size) return false;
    memcpy(out, br->data + br->pos, 8); 
    br->pos += 8;
    return true;
}
bool br_read_u16_inv(BufReader *br, u16 *out) {
    if (br->pos + 2 > br->size) return false;
    memcpy(out, br->data + br->pos, 2);
    *out = swap_16(*out);
    br->pos += 2;
    return true;
}
bool br_read_u32_inv(BufReader *br, u32 *out) {
    if (br->pos + 4 > br->size) return false;
    memcpy(out, br->data + br->pos, 4);
    *out = swap_32(*out);
    br->pos += 4;
    return true;
}
bool br_read_u64_inv(BufReader *br, u64 *out) {
    if (br->pos + 8 > br->size) return false;
    memcpy(out, br->data + br->pos, 8); 
    *out = swap_64(*out);
    br->pos += 8;
    return true;
}

bool br_read_bytes(BufReader *br, byte *dst, size_t n) {
    return br_read(br, (void*)dst, n);
}