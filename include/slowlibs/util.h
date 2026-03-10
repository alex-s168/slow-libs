#ifndef SLOWLIBS_UTIL_H
#define SLOWLIBS_UTIL_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#ifdef BITINT_MAXWIDTH
#if BITINT_MAXWIDTH >= 264
#define SLOWLIBS_BITINT_MAXWIDTH BITINT_MAXWIDTH
#endif
#elif defined(__BITINT_MAXWIDTH__)
#if __BITINT_MAXWIDTH__ >= 264
#define SLOWLIBS_BITINT_MAXWIDTH __BITINT_MAXWIDTH__
#endif
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define slowlibs_O0 __attribute__((optimize("O0")))
#else
#define slowlibs_O0 /**/
#endif

// TODO
#define slowlibs_ct_select(cond, a, b) ((cond) ? (a) : (b))

#define slowlibs_place(valty, val) ((valty*)((valty[]){val}))

typedef enum
{
  SLOWLIBS_ENDIAN_LITTLE = 0,
  SLOWLIBS_ENDIAN_BIG,
} slowlibs_endian;

#define SLOWLIBS_ENDIAN_HOST                                       \
  (((*((char*)slowlibs_place(int, 1))) == 0) ? SLOWLIBS_ENDIAN_BIG \
                                             : SLOWLIBS_ENDIAN_LITTLE)

#ifdef __cplusplus
extern "C" {
#endif

// Calls 'void slowlibs_memrevcpy_inplace(void* dst, size_t bytes)' when 'src == dst'
void slowlibs_memrevcpy(void* dst, void const* src, size_t bytes);

void slowlibs_memrevcpy_inplace(void* dst, size_t bytes);

#define slowlibs_swap_end(var) slowlibs_memrevcpy_inplace(&(var), sizeof(var))

static inline uint32_t slowlibs_rev_u32(uint32_t u)
{
  return ((u & 0xFF) << 24) | (((u >> 8) & 0xFF) << 16) |
         (((u >> 16) & 0xFF) << 8) | ((u >> 24) & 0xFF);
}

#ifdef __cplusplus
}
#endif

#endif
