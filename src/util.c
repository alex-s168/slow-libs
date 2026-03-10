#include "slowlibs/util.h"
#include <stdint.h>

void slowlibs_memrevcpy_inplace(void* dst, size_t bytes)
{
  uint8_t* dst_u8 = dst;
  for (size_t i = 0, j = bytes - 1; i < bytes >> 1; i++, j--) {
    uint8_t a = dst_u8[i];
    dst_u8[i] = dst_u8[j];
    dst_u8[j] = a;
  }
}

void slowlibs_memrevcpy(void* dst, void const* src, size_t bytes)
{
  if (dst == src) {
    slowlibs_memrevcpy_inplace(dst, bytes);
    return;
  }

  uint32_t* dst_u32 = dst;
  uint32_t const* src_last_u32 = (void*)((uint8_t const*)src + bytes);

  for (; bytes >= 4; bytes -= 4)
    *dst_u32++ = slowlibs_rev_u32(*--src_last_u32);

  uint8_t* dst_u8 = (void*)dst_u32;
  uint8_t const* src_last_u8 = (void const*)src_last_u32;

  for (; bytes > 0; bytes--)
    *dst_u8++ = *--src_last_u8;
}
