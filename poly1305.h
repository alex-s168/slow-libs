#ifndef SLOWCRYPT_POLY1305_H
#define SLOWCRYPT_POLY1305_H

#ifndef SLOWCRYPT_POLY1305_FUNC
#define SLOWCRYPT_POLY1305_FUNC /**/
#endif

#ifndef SLOWCRYPT_POLY1305_DONT_USE_BITINT
#include <limits.h>
#ifdef BITINT_MAXWIDTH
#if BITINT_MAXWIDTH >= 264
#define SLOWCRYPT_POLY1305_USE_BITINT
#endif
#elif defined(__BITINT_MAXWIDTH__)
#if __BITINT_MAXWIDTH__ >= 264
#define SLOWCRYPT_POLY1305_USE_BITINT
#endif
#endif
#endif

#ifdef SLOWCRYPT_POLY1305_USE_BITINT
#else
ERROR NOT YET IMPLEMENTED;
#ifdef SLOWCRYPT_POLY1305_IMPL
#define BIGINT_IMPLEMENTATION
#endif
#define BIGINT_FUNC SLOWCRYPT_POLY1305_FUNC
#define BIGINT_NAMESPACE(x) slowcrypt_poly1305_bigint_##x
#define BIGINT_UNDEF
#define BIGINT_NO_MALLOC
#define BIGINT_DEFAULT_LIMIT 33
#define BIGINT_WORD_WIDTH 1
#endif

#include <stdint.h>

typedef struct
{
#ifdef SLOWCRYPT_POLY1305_USE_BITINT
  unsigned _BitInt(128) r, s;
  unsigned _BitInt(136) acc;
  unsigned _BitInt(264) prod;
#else
  slowcrypt_poly1305_bigint_bigint r, s, acc, p, temp1, temp2;
#endif
} slowcrypt_poly1305;

/* the key buffer will be destroyed: used as scratch buffer */
SLOWCRYPT_POLY1305_FUNC void slowcrypt_poly1305_init(slowcrypt_poly1305* p,
                                                     uint8_t key[32]);

/* only the last block is allowed to be shorter than 16 bytes!
 * 
 * length has to be maximum 16
 */
SLOWCRYPT_POLY1305_FUNC void slowcrypt_poly1305_next_block(
    slowcrypt_poly1305* p,
    uint8_t const* data,
    unsigned int length);

/* also zeroizes memory */
SLOWCRYPT_POLY1305_FUNC void slowcrypt_poly1305_finish(slowcrypt_poly1305* p,
                                                       uint8_t out[16]);

#ifdef SLOWCRYPT_POLY1305_IMPL

#ifdef SLOWCRYPT_POLY1305_USE_BITINT

static unsigned _BitInt(136) slowcrypt_poly1305_from_le(uint8_t const* buf,
                                                        unsigned int buf_len,
                                                        uint8_t top)
{
  unsigned int i;
  unsigned _BitInt(136) out = (unsigned _BitInt(136))top << (buf_len * 8);
  for (i = 0; i < buf_len; i++)
    out |= (unsigned _BitInt(136))buf[i] << (i * 8);
  return out;
}

SLOWCRYPT_POLY1305_FUNC void slowcrypt_poly1305_init(slowcrypt_poly1305* p,
                                                     uint8_t key[32])
{
  key[3] &= 15;
  key[7] &= 15;
  key[11] &= 15;
  key[15] &= 15;
  key[4] &= 252;
  key[8] &= 252;
  key[12] &= 252;
  p->r = (unsigned _BitInt(128))slowcrypt_poly1305_from_le(key, 16, 0);
  p->s = (unsigned _BitInt(128))slowcrypt_poly1305_from_le(&key[16], 16, 0);
  p->acc = 0;
}

SLOWCRYPT_POLY1305_FUNC void slowcrypt_poly1305_next_block(
    slowcrypt_poly1305* p,
    uint8_t const* data,
    unsigned int length)
{
  if (length == 0)
    return;
  p->prod = p->acc + slowcrypt_poly1305_from_le(data, length, 0x01);
  p->prod *= p->r;
  p->prod %= ((unsigned _BitInt(136))1 << 130) - (unsigned _BitInt(136))5;
  p->acc = p->prod;
}

SLOWCRYPT_POLY1305_FUNC void slowcrypt_poly1305_finish(slowcrypt_poly1305* p,
                                                       uint8_t out[16])
{
  unsigned int i;
  p->acc += p->s;
  for (i = 0; i < 16; i++)
    out[i] = (uint8_t)((p->acc >> (i * 8)));

  for (i = 0; i < sizeof(slowcrypt_poly1305); i++)
    ((char*)p)[i] = 0;
}

#else
static void slowcrypt_poly1305_from_le(slowcrypt_poly1305_bigint_bigint* out,
                                       slowcrypt_poly1305_bigint_bigint* temp,
                                       uint8_t const* buf,
                                       unsigned int buf_len,
                                       uint8_t top)
{
  slowcrypt_poly1305_bigint_from_u8(temp, top);
  for (; buf_len-- > 0; buf++) {
    slowcrypt_poly1305_bigint_from_u8(temp, *buf);
    slowcrypt_poly1305_bigint_lshift_overwrite(out, 8);
    slowcrypt_poly1305_bigint_inc(out, temp);
  }
}

SLOWCRYPT_POLY1305_FUNC void slowcrypt_poly1305_init(slowcrypt_poly1305* p,
                                                     uint8_t key[32])
{
  (void)slowcrypt_poly1305_bigint_from_cstring(
      &p->p, "3fffffffffffffffffffffffffffffffb", 16);
  key[3] &= 15;
  key[7] &= 15;
  key[11] &= 15;
  key[15] &= 15;
  key[4] &= 252;
  key[8] &= 252;
  key[12] &= 252;
  slowcrypt_poly1305_from_le(&p->r, &p->temp1, key, 16, 0);
  slowcrypt_poly1305_from_le(&p->s, &p->temp1, &key[16], 16, 0);
}

/* only the last block is allowed to be shorter than 16 bytes! */
SLOWCRYPT_POLY1305_FUNC void slowcrypt_poly1305_next_block(
    slowcrypt_poly1305* p,
    uint8_t const* data,
    unsigned int length)
{
  if (length == 0)
    return;
  slowcrypt_poly1305_from_le(&p->temp1, &p->temp2, data, length, 0x01);
  slowcrypt_poly1305_bigint_inc(&p->acc, &p->temp1);
  slowcrypt_poly1305_bigint_mul(&p->acc, &p->acc, &p->r);
  slowcrypt_poly1305_bigint_div_mod(&p->temp1, &p->acc, &p->acc, &p->p);
}

SLOWCRYPT_POLY1305_FUNC void slowcrypt_poly1305_finish(slowcrypt_poly1305* p,
                                                       uint8_t out[16])
{
  unsigned int i;
  slowcrypt_poly1305_bigint_inc(&p->acc, &p->s);
  for (i = 0; i < 16; i++)
    out[i] = p->acc.words[i];
  for (i = 0; i < sizeof(slowcrypt_poly1305); i++)
    ((char*)p)[i] = 0;
}
#endif

#endif

#endif
