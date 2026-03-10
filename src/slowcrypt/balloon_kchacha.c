
// run modulo operation for pseudo-random selection on whole buffer slice, like in paper
#define FULL_MOD 0

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "slowlibs/chacha20.h"
#if FULL_MOD
#include "slowlibs/fixed_bigint.h"
#endif
#include "slowlibs/util.h"

static void* cat_u32(void* buf, uint32_t cnt)
{
  memcpy(buf, &cnt, 4);
  if (SLOWLIBS_ENDIAN_HOST != SLOWLIBS_ENDIAN_LITTLE)
    slowlibs_memrevcpy_inplace(buf, 4);

  return buf + 4;
}

static void* cat_inc_u32(void* buf, uint32_t* cnt)
{
  memcpy(buf, cnt, 4);
  if (SLOWLIBS_ENDIAN_HOST != SLOWLIBS_ENDIAN_LITTLE)
    slowlibs_memrevcpy_inplace(buf, 4);

  (*cnt)++;
  return buf + 4;
}

static void* cat_buf(void* buf, void const* src, unsigned num)
{
  memcpy(buf, src, num);
  return buf + num;
}

int slowcrypt_balloon_kchacha(uint8_t out[32],
                              uint8_t const protocol_constant[16],
                              uint8_t const password[],
                              unsigned password_len,
                              uint8_t const salt[],
                              unsigned salt_len,
                              unsigned buffer_size,
                              unsigned balloon_rounds,
                              unsigned kchacha_rounds)
{
  uint8_t *buf, *blkbuf;
  unsigned m, t, i, len;
  uint32_t cnt = 0, random_buf_id;
#if FULL_MOD
  slowlib_fbig_var(32 * 8, yetanotherbuffer);
  slowlib_fbig_var(8, somehowneedanotherbuffer);
#else
  uint8_t yetanotherbuffer[32];
#endif

  buffer_size /= 32;
  buf = malloc(buffer_size * 32);
  if (!buf)
    return 1;

  m = password_len + salt_len + 4 + 64;
  blkbuf = malloc(m);
  if (!blkbuf) {
    free(buf);
    return 1;
  }

  // Step 1: Expand input into buffer
  cat_buf(cat_buf(cat_inc_u32(blkbuf, &cnt), password, password_len), salt,
          salt_len);
  slowcrypt_kchacha(&buf[0], protocol_constant, blkbuf,
                    4 + password_len + salt_len, kchacha_rounds);

  for (m = 1; m < buffer_size; m++) {
    cat_buf(cat_inc_u32(blkbuf, &cnt), &buf[(m - 1) * 32], 32);
    slowcrypt_kchacha(&buf[m * 32], protocol_constant, blkbuf, 4 + 32,
                      kchacha_rounds);
  }

  // Step 2: Mix buffer contents
  for (t = 0; t < balloon_rounds; t++) {
    for (m = 0; m < buffer_size; m++) {
      // Step 2a: hash last and current blocks
      cat_buf(cat_buf(cat_inc_u32(blkbuf, &cnt),
                      &buf[((m - 1) % buffer_size) * 32], 32),
              &buf[m * 32], 32);
      slowcrypt_kchacha(&buf[m * 32], protocol_constant, blkbuf, 4 + 32 + 32,
                        kchacha_rounds);

      // Step 2b: Hash in pseudorandom chosen blocks
      for (i = 0; i < 3; i++) {
        // fuck you (you, the person who has to suffer reading this code)
        len = cat_u32(cat_u32(cat_u32(cat_buf(cat_inc_u32(blkbuf, &cnt), salt,
                                              salt_len),
                                      t),
                              m),
                      i) -
              (void*)blkbuf;
#if FULL_MOD
        slowcrypt_kchacha((void*)yetanotherbuffer, protocol_constant, blkbuf,
                          len, kchacha_rounds);

        slowlib_fbig_zext_scalar(somehowneedanotherbuffer, buffer_size);
        slowlib_fbig_umod(yetanotherbuffer, yetanotherbuffer,
                          somehowneedanotherbuffer);
        if (SLOWLIBS_ENDIAN_HOST != SLOWLIBS_ENDIAN_LITTLE) {
          slowlibs_memrevcpy_inplace(yetanotherbuffer, sizeof yetanotherbuffer);
        }
        random_buf_id = *(uint32_t*)(void*)yetanotherbuffer;
#else
        slowcrypt_kchacha((void*)yetanotherbuffer, protocol_constant, blkbuf,
                          len, kchacha_rounds);
        if (SLOWLIBS_ENDIAN_HOST != SLOWLIBS_ENDIAN_LITTLE) {
          slowlibs_memrevcpy_inplace(yetanotherbuffer, 4);
        }
        random_buf_id = *(uint32_t*)(void*)yetanotherbuffer % buffer_size;
#endif

        len = cat_buf(cat_buf(cat_inc_u32(blkbuf, &cnt), &buf[32 * m], 32),
                      &buf[random_buf_id * 32], 32) -
              (void*)blkbuf;
        slowcrypt_kchacha(&buf[32 * m], protocol_constant, blkbuf, len,
                          kchacha_rounds);
      }
    }
  }

  for (m = 0; m < 32; m++)
    out[m] = buf[(buffer_size - 1) * 32 + m];
  free(buf);

  for (i = 0; i < 32; i++)
    ((volatile uint8_t*)yetanotherbuffer)[i] = 0;

  return 0;
}
