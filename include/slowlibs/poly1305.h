
/*
 * Copyright (c) 2026 Alexander Nutz
 * 0BSD licensed, see below documentation
 *
 * Latest version can be found at:
 * https://git.vxcc.dev/alexander.nutz/slow-libs
 *
 *
 * ======== Poly1305 Message Authentication Code ========
 *
 * Security considerations:
 * - manually zeroize memory (depending on your application)
 * - timing attacks:
 *   To prevent timing attacks. Make sure you are using a non-optimizing compiler,
 *   and do NOT define SLOWCRYPT_ALLOW_TIMING_ATTACKS.
 *
 *
 * Configuration options:
 * - SLOWCRYPT_POLY1305_IMPL
 * - SLOWCRYPT_POLY1305_FUNC
 *     will be used in front of every function definition / declaration
 * - SLOWCRYPT_ALLOW_TIMING_ATTACKS
 *     Uses faster non-constant time implementations.
 *     NOTE: even when this is not enabled, you have to use a non-optimizing compiler, to prevent timing attacks.
 * - SLOWCRYPT_BITINT_IS_CONSTANT_TIME
 *     Define this if you know that _BitInt operations are constant time on your platform.
 * - SLOWCRYPT_POLY1305_DONT_USE_BITINT
 *     do not use _BitInt, even if it is known to be constant time
 *
 *
 * Compatibility:
 *   requires only a C99 compiler.
 */

/*
 * Copyright (C) 2026 by Alexander Nutz <alexander.nutz@vxcc.dev>
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 * NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifndef SLOWCRYPT_POLY1305_H
#define SLOWCRYPT_POLY1305_H

#if defined(SLOWCRYPT_CHACHA20_IMPL) && defined(SLOWCRYPT_STATICALLY_LINKED)
}}} ERROR ERROR ERROR {{{
#endif

#ifndef SLOWCRYPT_POLY1305_FUNC
#define SLOWCRYPT_POLY1305_FUNC /**/
#endif

#if !defined(SLOWCRYPT_BITINT_IS_CONSTANT_TIME) && \
    !defined(SLOWCRYPT_ALLOW_TIMING_ATTACKS)
#ifndef SLOWCRYPT_POLY1305_DONT_USE_BITINT
#define SLOWCRYPT_POLY1305_DONT_USE_BITINT
#endif
#endif

#include "util.h"

#ifndef SLOWCRYPT_POLY1305_DONT_USE_BITINT
#ifdef SLOWLIBS_BITINT_MAXWIDTH
#if SLOWLIBS_BITINT_MAXWIDTH >= 264
#define SLOWCRYPT_POLY1305_USE_BITINT
#endif
#endif
#endif

#ifndef slowcrypt_timing_sensitive
#ifdef SLOWCRYPT_ALLOW_TIMING_ATTACKS
#define slowcrypt_timing_sensitive /**/
#else
#define slowcrypt_timing_sensitive slowlibs_O0
#endif
#endif

#ifndef SLOWCRYPT_POLY1305_USE_BITINT
#ifndef SLOWCRYPT_ALLOW_TIMING_ATTACKS
#define SLOWLIBS_FBIG_CONSTANT_TIME
#endif
#include "fixed_bigint.h"
#endif

#include <stdint.h>

typedef struct
{
#ifdef SLOWCRYPT_POLY1305_USE_BITINT
  unsigned _BitInt(128) r, s;
  unsigned _BitInt(136) acc;
  unsigned _BitInt(264) prod;
#else
  slowlib_fbig_var(128, r);
  slowlib_fbig_var(128, s);
  slowlib_fbig_var(136, acc);
  slowlib_fbig_var(264, prod);
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

static unsigned _BitInt(136) slowcrypt_timing_sensitive
    slowcrypt_poly1305_from_le(uint8_t const* buf,
                               unsigned int buf_len,
                               uint8_t top)
{
  unsigned int i;
  unsigned _BitInt(136) out = (unsigned _BitInt(136))top << (buf_len * 8);
  for (i = 0; i < buf_len; i++)
    out |= (unsigned _BitInt(136))buf[i] << (i * 8);
  return out;
}

SLOWCRYPT_POLY1305_FUNC void slowcrypt_timing_sensitive
slowcrypt_poly1305_init(slowcrypt_poly1305* p, uint8_t key[32])
{
  key[3] &= 0x0f;
  key[4] &= 0xfc;
  key[7] &= 0x0f;
  key[8] &= 0xfc;
  key[11] &= 0x0f;
  key[12] &= 0xfc;
  key[15] &= 0x0f;
  p->r = (unsigned _BitInt(128))slowcrypt_poly1305_from_le(key, 16, 0);
  p->s = (unsigned _BitInt(128))slowcrypt_poly1305_from_le(&key[16], 16, 0);
  p->acc = 0;
}

SLOWCRYPT_POLY1305_FUNC void slowcrypt_timing_sensitive
slowcrypt_poly1305_next_block(slowcrypt_poly1305* p,
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

SLOWCRYPT_POLY1305_FUNC void slowcrypt_timing_sensitive
slowcrypt_poly1305_finish(slowcrypt_poly1305* p, uint8_t out[16])
{
  unsigned int i;
  p->acc += p->s;
  for (i = 0; i < 16; i++)
    out[i] = (uint8_t)((p->acc >> (i * 8)));

  for (i = 0; i < sizeof(slowcrypt_poly1305); i++)
    ((volatile char*)p)[i] = 0;
}

#else

static void slowcrypt_timing_sensitive
slowcrypt_poly1305_from_le(slowlib_fbig_part outp[slowlib_fbig(136)],
                           uint8_t const* buf,
                           unsigned int buf_len,
                           uint8_t top)
{
  unsigned int i;

  slowlib_fbig_var(136, out);
  slowlib_fbig_zext_scalar(out, top);
  slowlib_fbig_shl(out, out, buf_len * 8);

  slowlib_fbig_var(136, byte_big);
  for (i = 0; i < buf_len; i++) {
    slowlib_fbig_zext_scalar(byte_big, buf[i]);
    slowlib_fbig_shl(byte_big, byte_big, i * 8);
    slowlib_fbig_or(out, out, byte_big);
  }

  slowlib_fbig_write(outp, out);
}

SLOWCRYPT_POLY1305_FUNC void slowcrypt_timing_sensitive
slowcrypt_poly1305_init(slowcrypt_poly1305* p, uint8_t key[32])
{
  key[3] &= 0x0f;
  key[4] &= 0xfc;
  key[7] &= 0x0f;
  key[8] &= 0xfc;
  key[11] &= 0x0f;
  key[12] &= 0xfc;
  key[15] &= 0x0f;
  slowcrypt_poly1305_from_le(p->r, key, 16, 0);
  slowcrypt_poly1305_from_le(p->s, &key[16], 16, 0);
  slowlib_fbig_zext_scalar(p->acc, 0);
}

SLOWCRYPT_POLY1305_FUNC void slowcrypt_timing_sensitive
slowcrypt_poly1305_next_block(slowcrypt_poly1305* p,
                              uint8_t const* data,
                              unsigned int length)
{
  if (length == 0)
    return;

  slowlib_fbig_var(136, temp);
  slowcrypt_poly1305_from_le(temp, data, length, 0x01);
  slowlib_fbig_add(temp, p->acc, temp);
  slowlib_fbig_mul(p->prod, temp, p->r);

  // (1 << 130) - 5
  slowlib_fbig_const(136, P,
                     {slowlib_fbig_const_part_u64(0xfffffffffffffffb),
                      slowlib_fbig_const_part_u64(0xffffffffffffffff),
                      slowlib_fbig_const_part_u64(0x3)});

  slowlib_fbig_umod(p->prod, p->prod, P);
  slowlib_fbig_trunc(p->acc, p->prod);
}

SLOWCRYPT_POLY1305_FUNC void slowcrypt_timing_sensitive
slowcrypt_poly1305_finish(slowcrypt_poly1305* p, uint8_t out[16])
{
  unsigned int i;
  slowlib_fbig_add(p->acc, p->acc, p->s);
  for (i = 0; i < 16; i++)
    out[i] = slowlib_fbig_as_bytes(p->acc)[i];

  for (i = 0; i < sizeof(slowcrypt_poly1305); i++)
    ((volatile char*)p)[i] = 0;
}

#endif

#endif

#endif
