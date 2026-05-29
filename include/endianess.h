#ifndef ENDIANESS_H
#define ENDIANESS_H

// endianness
#if defined(__linux__) || defined(__CYGWIN__)
    #include <endian.h>
#elif defined(__Apple__)
    #include <machine/endian.h>
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    #include <sys/endian.h>
#endif

#ifndef __ORDER_LITTLE_ENDIAN__
    #define __ORDER_LITTLE_ENDIAN__ 1234
#endif
#ifndef __ORDER_BIG_ENDIAN__
    #define __ORDER_BIG_ENDIAN__ 4321
#endif

#ifndef __BYTE_ORDER__
    #if defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(_MIPSEB)
        #define __BYTE_ORDER__ __ORDER_BIG_ENDIAN__
    #else
        #define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
    #endif
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define IS_LITLE_ENDIAN
#elif __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    #error "Unknown archictecture endianness! You are on a weird-ass system."
#endif

// swappers
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

#endif