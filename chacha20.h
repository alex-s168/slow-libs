/*
 * Copyright (c) 2025 Alexander Nutz
 * MIT licensed, see below documentation
 *
 * Latest version can be found at:
 * https://gitea.vxcc.dev/alexander.nutz/slow-libs
 *
 *
 * ======== ChaCha20 stream cihper ========
 *
 * Security considerations:
 * - manually zeroize memory (depending on your application)
 * - length extension attack:
 *   ChaCha20 is only a stream cihper, and, like AES,
 *   does NOT prevent against length extension attacks.
 *   Consider using ChaCha20-Poly1305 instead.
 *
 *
 * Configuration options:
 * - SLOWCRYPT_CHACHA20_IMPL
 * - SLOWCRYPT_CHACHA20_FUNC
 *     will be used in front of every function definition / declaration
 * - SLOWCRYPT_CHACHA20_UINT32
 *     if this is not set, will include <stdint.h>, and use `uint32_t`
 *
 *
 * Compatibility:
 *   requires only a C89 compiler.
 *
 *
 * Usage example 1: en-/de- crypt blocks of data
 *     slowcrypt_chacha20 state[2];
 *     uint32_t ctr = 1;
 *     char buf[64];
 *
 *     iterate over blocks {
 *       copy block into buf (can pad with zeros)
 *       slowcrypt_chacha20_block(state, key, ctr, nonce, buf);
 *       ctr += 1;
 *     }
 *
 *     # optionally zeroize memory
 *     slowcrypt_chacha20_deinit(&state[0]);
 *     slowcrypt_chacha20_deinit(&state[1]);
 *     bzero(buf, 64);
 *
 *
 * Usage example 2: CSPRNG (cryptographically secure pseudo random number generator)
 *     slowcrypt_chacha20 state[2];
 *     uint32_t ctr = 1;
 *     char buf[64];
 *
 *     while need random numbers {
 *       slowcrypt_chacha20_init(state, key, block_ctr, nonce);
 *       slowcrypt_chacha20_run(state, &state[1], 20);
 *       slowcrypt_chacha20_serialize(buf, state);
 *       yield buf
 *       ctr += 1;
 *     }
 *
 *     # optionally zeroize memory
 *     slowcrypt_chacha20_deinit(&state[0]);
 *     slowcrypt_chacha20_deinit(&state[1]);
 *     bzero(buf, 64);
 *
 */

/*
 * Copyright (c) 2025 Alexander Nutz
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions: The above copyright
 * notice and this permission notice shall be included in all copies or
 * substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS",
 * WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SLOWCRYPT_CHACHA20_H
#define SLOWCRYPT_CHACHA20_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SLOWCRYPT_CHACHA20_UINT32
#include <stdint.h>
#define SLOWCRYPT_CHACHA20_UINT32 uint32_t
#endif

#ifndef SLOWCRYPT_CHACHA20_FUNC
#define SLOWCRYPT_CHACHA20_FUNC /**/
#endif

typedef struct
{
  SLOWCRYPT_CHACHA20_UINT32 state[16];
} slowcrypt_chacha20;

/*
 * initialize state, run 20 iterations, serialize, xor data inplace
 *
 * does NOT zeroize states! zeroize manually when done.
 */
SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_block(
    slowcrypt_chacha20 state[2],
    char const key[32],
    SLOWCRYPT_CHACHA20_UINT32 block_ctr,
    char const nonce[12],
    char data[64]);

/* call this to zero out memory */
SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_deinit(
    slowcrypt_chacha20* state);

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_init(
    slowcrypt_chacha20* state,
    char const key[32],
    SLOWCRYPT_CHACHA20_UINT32 block_ctr,
    char const nonce[12]);

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_serialize(
    char buf[64],
    slowcrypt_chacha20 const* state);

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_serialize_xor(
    char buf[64],
    slowcrypt_chacha20 const* state);

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_run(slowcrypt_chacha20* state,
                                                    slowcrypt_chacha20* swap,
                                                    int num_rounds);

SLOWCRYPT_CHACHA20_FUNC SLOWCRYPT_CHACHA20_UINT32
slowcrypt_chacha20_read_ul32(char const* buf);

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_write_ul32(
    char* buf,
    SLOWCRYPT_CHACHA20_UINT32 val);

#define SLOWCRYPT_CHACHA20_LAST32(n, bits) \
  (((SLOWCRYPT_CHACHA20_UINT32)(n)) >> (32 - (bits)))

#define SLOWCRYPT_CHACHA20_ROL32(n, by)         \
  ((((SLOWCRYPT_CHACHA20_UINT32)(n)) << (by)) | \
   SLOWCRYPT_CHACHA20_LAST32((n), (by)))

#define SLOWCRYPT_CHACHA20_QROUND(state, a, b, c, d)   \
  do {                                                 \
    state[a] += state[b];                              \
    state[d] ^= state[a];                              \
    state[d] = SLOWCRYPT_CHACHA20_ROL32(state[d], 16); \
                                                       \
    state[c] += state[d];                              \
    state[b] ^= state[c];                              \
    state[b] = SLOWCRYPT_CHACHA20_ROL32(state[b], 12); \
                                                       \
    state[a] += state[b];                              \
    state[d] ^= state[a];                              \
    state[d] = SLOWCRYPT_CHACHA20_ROL32(state[d], 8);  \
                                                       \
    state[c] += state[d];                              \
    state[b] ^= state[c];                              \
    state[b] = SLOWCRYPT_CHACHA20_ROL32(state[b], 7);  \
  } while (0)

#ifdef SLOWCRYPT_CHACHA20_IMPL

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_deinit(
    slowcrypt_chacha20* state)
{
  int i;
  for (i = 0; i < 16; i++)
    *(volatile int*)&state->state[i] = 0;
}

SLOWCRYPT_CHACHA20_FUNC SLOWCRYPT_CHACHA20_UINT32
slowcrypt_chacha20_read_ul32(char const* buf)
{
  SLOWCRYPT_CHACHA20_UINT32 o =
      (SLOWCRYPT_CHACHA20_UINT32)((uint8_t const*)buf)[0];
  o |= (SLOWCRYPT_CHACHA20_UINT32)((uint8_t const*)buf)[1] << 8;
  o |= (SLOWCRYPT_CHACHA20_UINT32)((uint8_t const*)buf)[2] << 16;
  o |= (SLOWCRYPT_CHACHA20_UINT32)((uint8_t const*)buf)[3] << 24;
  return o;
}

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_write_ul32(
    char* buf,
    SLOWCRYPT_CHACHA20_UINT32 val)
{
  ((uint8_t*)buf)[0] = (uint8_t)(val & 0xFF);
  ((uint8_t*)buf)[1] = (uint8_t)((val >> 8) & 0xFF);
  ((uint8_t*)buf)[2] = (uint8_t)((val >> 16) & 0xFF);
  ((uint8_t*)buf)[3] = (uint8_t)((val >> 24) & 0xFF);
}

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_init(
    slowcrypt_chacha20* state,
    char const key[32],
    SLOWCRYPT_CHACHA20_UINT32 block_ctr,
    char const nonce[12])
{
  int i;

  state->state[0] = 0x61707865;
  state->state[1] = 0x3320646e;
  state->state[2] = 0x79622d32;
  state->state[3] = 0x6b206574;

  for (i = 0; i < 8; i++)
    state->state[4 + i] = slowcrypt_chacha20_read_ul32(&key[i * 4]);

  state->state[12] = block_ctr;

  for (i = 0; i < 3; i++)
    state->state[13 + i] = slowcrypt_chacha20_read_ul32(&nonce[i * 4]);
}

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_serialize(
    char buf[64],
    slowcrypt_chacha20 const* state)
{
  int i;
  for (i = 0; i < 16; i++)
    slowcrypt_chacha20_write_ul32(&buf[i * 4], state->state[i]);
}

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_serialize_xor(
    char buf[64],
    slowcrypt_chacha20 const* state)
{
  char swp[4];
  int i, j;

  for (i = 0; i < 16; i++) {
    slowcrypt_chacha20_write_ul32(swp, state->state[i]);
    for (j = 0; j < 4; j++)
      buf[i * 4 + j] ^= swp[j];
  }
}

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_run(slowcrypt_chacha20* state,
                                                    slowcrypt_chacha20* swap,
                                                    int num_rounds)
{
  int i;

  for (i = 0; i < 16; i++)
    swap->state[i] = state->state[i];

  for (i = 0; i < num_rounds; i++) {
    if (i % 2 == 0) {
      /* column round */
      SLOWCRYPT_CHACHA20_QROUND(state->state, 0, 4, 8, 12);
      SLOWCRYPT_CHACHA20_QROUND(state->state, 1, 5, 9, 13);
      SLOWCRYPT_CHACHA20_QROUND(state->state, 2, 6, 10, 14);
      SLOWCRYPT_CHACHA20_QROUND(state->state, 3, 7, 11, 15);
    } else {
      /* diagonal round */
      SLOWCRYPT_CHACHA20_QROUND(state->state, 0, 5, 10, 15);
      SLOWCRYPT_CHACHA20_QROUND(state->state, 1, 6, 11, 12);
      SLOWCRYPT_CHACHA20_QROUND(state->state, 2, 7, 8, 13);
      SLOWCRYPT_CHACHA20_QROUND(state->state, 3, 4, 9, 14);
    }
  }

  for (i = 0; i < 16; i++)
    state->state[i] += swap->state[i];
}

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_block(
    slowcrypt_chacha20 state[2],
    char const key[32],
    SLOWCRYPT_CHACHA20_UINT32 block_ctr,
    char const nonce[12],
    char data[64])
{
  slowcrypt_chacha20_init(state, key, block_ctr, nonce);
  slowcrypt_chacha20_run(state, &state[1], 20);
  slowcrypt_chacha20_serialize_xor(data, state);
}

#endif

#ifdef __cplusplus
}
#endif

#endif
