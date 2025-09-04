/* define SLOWGRAPH_IMPL */

#ifndef SLOWCRYPT_CHACHA20_H
#define SLOWCRYPT_CHACHA20_H

#include <stdint.h>

#ifndef SLOWCRYPT_CHACHA20_FUNC
#define SLOWCRYPT_CHACHA20_FUNC /**/
#endif

typedef struct {
  uint32_t state[16];
} slowcrypt_chacha20;

/*
 * initialize state, run 20 iterations, serialize, xor data inplace
 *
 * does NOT zeroize states! zeroize manually when done.
 */
SLOWCRYPT_CHACHA20_FUNC void
slowcrypt_chacha20_block(slowcrypt_chacha20 state[2], char const key[32],
                         uint32_t block_ctr, char const nonce[12],
                         char data[64]);

/* call this to zero out memory */
SLOWCRYPT_CHACHA20_FUNC void
slowcrypt_chacha20_deinit(slowcrypt_chacha20 *state);

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_init(slowcrypt_chacha20 *state,
                                                     char const key[32],
                                                     uint32_t block_ctr,
                                                     char const nonce[12]);

SLOWCRYPT_CHACHA20_FUNC void
slowcrypt_chacha20_serialize(char buf[64], slowcrypt_chacha20 const *state);

SLOWCRYPT_CHACHA20_FUNC void
slowcrypt_chacha20_serialize_xor(char buf[64], slowcrypt_chacha20 const *state);

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_run(slowcrypt_chacha20 *state,
                                                    slowcrypt_chacha20 *swap,
                                                    int num_rounds);

SLOWCRYPT_CHACHA20_FUNC uint32_t slowcrypt_chacha20_read_ul32(char const *buf);

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_write_ul32(char *buf,
                                                           uint32_t val);

#define SLOWCRYPT_CHACHA20_LAST32(n, bits) (((uint32_t)(n)) >> (32 - (bits)))

#define SLOWCRYPT_CHACHA20_ROL32(n, by)                                        \
  ((((uint32_t)(n)) << (by)) | SLOWCRYPT_CHACHA20_LAST32((n), (by)))

#define SLOWCRYPT_CHACHA20_QROUND(state, a, b, c, d)                           \
  do {                                                                         \
    state[a] += state[b];                                                      \
    state[d] ^= state[a];                                                      \
    state[d] = SLOWCRYPT_CHACHA20_ROL32(state[d], 16);                         \
                                                                               \
    state[c] += state[d];                                                      \
    state[b] ^= state[c];                                                      \
    state[b] = SLOWCRYPT_CHACHA20_ROL32(state[b], 12);                         \
                                                                               \
    state[a] += state[b];                                                      \
    state[d] ^= state[a];                                                      \
    state[d] = SLOWCRYPT_CHACHA20_ROL32(state[d], 8);                          \
                                                                               \
    state[c] += state[d];                                                      \
    state[b] ^= state[c];                                                      \
    state[b] = SLOWCRYPT_CHACHA20_ROL32(state[b], 7);                          \
  } while (0)

#ifdef SLOWCRYPT_CHACHA20_IMPL

SLOWCRYPT_CHACHA20_FUNC void
slowcrypt_chacha20_deinit(slowcrypt_chacha20 *state) {
  int i;
  for (i = 0; i < 16; i++)
    *(volatile int *)&state->state[i] = 0;
}

SLOWCRYPT_CHACHA20_FUNC uint32_t slowcrypt_chacha20_read_ul32(char const *buf) {
  uint32_t o = (uint32_t)((uint8_t const *)buf)[0];
  o |= (uint32_t)((uint8_t const *)buf)[1] << 8;
  o |= (uint32_t)((uint8_t const *)buf)[2] << 16;
  o |= (uint32_t)((uint8_t const *)buf)[3] << 24;
  return o;
}

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_write_ul32(char *buf,
                                                           uint32_t val) {
  ((uint8_t *)buf)[0] = (uint8_t)(val & 0xFF);
  ((uint8_t *)buf)[1] = (uint8_t)((val >> 8) & 0xFF);
  ((uint8_t *)buf)[2] = (uint8_t)((val >> 16) & 0xFF);
  ((uint8_t *)buf)[3] = (uint8_t)((val >> 24) & 0xFF);
}

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_init(slowcrypt_chacha20 *state,
                                                     char const key[32],
                                                     uint32_t block_ctr,
                                                     char const nonce[12]) {
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

SLOWCRYPT_CHACHA20_FUNC void
slowcrypt_chacha20_serialize(char buf[64], slowcrypt_chacha20 const *state) {
  int i;
  for (i = 0; i < 16; i++)
    slowcrypt_chacha20_write_ul32(&buf[i * 4], state->state[i]);
}

SLOWCRYPT_CHACHA20_FUNC void
slowcrypt_chacha20_serialize_xor(char buf[64],
                                 slowcrypt_chacha20 const *state) {
  char swp[4];
  int i, j;

  for (i = 0; i < 16; i++) {
    slowcrypt_chacha20_write_ul32(swp, state->state[i]);
    for (j = 0; j < 4; j++)
      buf[i * 4 + j] ^= swp[j];
  }
}

SLOWCRYPT_CHACHA20_FUNC void slowcrypt_chacha20_run(slowcrypt_chacha20 *state,
                                                    slowcrypt_chacha20 *swap,
                                                    int num_rounds) {
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

SLOWCRYPT_CHACHA20_FUNC void
slowcrypt_chacha20_block(slowcrypt_chacha20 state[2], char const key[32],
                         uint32_t block_ctr, char const nonce[12],
                         char data[64]) {
  slowcrypt_chacha20_init(state, key, block_ctr, nonce);
  slowcrypt_chacha20_run(state, &state[1], 20);
  slowcrypt_chacha20_serialize_xor(data, state);
}

#endif

#endif
