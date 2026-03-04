#include "slowlibs/sha3.h"
#include "slowlibs/slowcrypt.h"
#include "slowlibs/util.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define slowlib_fbig_part uint32_t
#define slowlib_fbig_double_part uint64_t
#include "slowlibs/fixed_bigint.h"

#include "sha3_gen_rc.h"

// see https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.202.pdf

// we store state lane-wise, as u32 arrays, where bit 0 is z=0
//
// state is:
//   uint32_t state[5][5][width/32]
//                  ^  ^  ^
//                  x  y  zpart
//
// which means it is indexed:
//   state[x][y][zpart]
//
// which means that index calculation is:
//   i(x,y,z) = zpart * 25 + y * 5 + x

#define part_idx(x, y, z) ((z) * 25 + (y) * 5 + (x))
#define zpart_mask(z) (1U << ((z) % 32))
#define bit_get(x, bit) (((x) >> (bit)) & 1U)
#define bit_replace(x, bit, val) ((x) ^ ((-(val) ^ (x)) & (1U << (bit))))

static uint32_t read_lane_part(uint8_t const from[4])
{
  return (uint32_t)from[0] | ((uint32_t)from[1] << 8) |
         ((uint32_t)from[2] << 16) | ((uint32_t)from[3] << 24);
}

static void write_lane_part(uint8_t to[4], uint32_t val)
{
  to[0] = (uint8_t)(val & 0xFF);
  to[1] = (uint8_t)((val >> 8) & 0xFF);
  to[2] = (uint8_t)((val >> 16) & 0xFF);
  to[3] = (uint8_t)((val >> 24) & 0xFF);
}

// contract: from len >= width / 32 * 25
static void read_state(uint32_t* out, uint8_t const* from, size_t width)
{
  for (size_t zpart = 0; zpart < width / 32; zpart++) {
    for (size_t y = 0; y < 5; y++) {
      for (size_t x = 0; x < 5; x++) {
        out[part_idx(x, y, zpart)] =
            read_lane_part(from + part_idx(x, y, zpart) * 4);
      }
    }
  }
}

// contract: to len >= width / 32 * 25
static void write_state(uint8_t* to, uint32_t const* from, size_t width)
{
  for (size_t zpart = 0; zpart < width / 32; zpart++) {
    for (size_t y = 0; y < 5; y++) {
      for (size_t x = 0; x < 5; x++) {
        write_lane_part(to + part_idx(x, y, zpart) * 4,
                        from[part_idx(x, y, zpart)]);
      }
    }
  }
}

static uint32_t column_reduce_xor_part(uint32_t const* state,
                                       size_t x,
                                       size_t zpart)
{
  return state[part_idx(x, 0, zpart)] ^ state[part_idx(x, 1, zpart)] ^
         state[part_idx(x, 2, zpart)] ^ state[part_idx(x, 3, zpart)] ^
         state[part_idx(x, 4, zpart)];
}

static void theta(uint32_t* out, uint32_t const* in, size_t width)
{
  size_t np = width / 32;
  for (size_t x = 0; x < 5; x++) {
    size_t x_left = (x - 1) % 5;
    size_t x_right = (x + 1) % 5;
    for (size_t zpart = 0; zpart < np; zpart++) {
      size_t zpart_below = (zpart - 1) % np;

      uint32_t left = column_reduce_xor_part(in, x_left, zpart);
      uint32_t right = column_reduce_xor_part(in, x_right, zpart);
      uint32_t right_part_below =
          column_reduce_xor_part(in, x_right, zpart_below);
      // stored lane-wise where bit 0 is z=0
      uint32_t right_below = (right >> 1) | (right_part_below << 31);

      uint32_t d = left ^ right_below;
      for (size_t y = 0; y < 5; y++) {
        out[part_idx(x, y, zpart)] = in[part_idx(x, y, zpart)] ^ d;
      }
    }
  }
}

static const uint16_t RHO_STEP_MAPPINGS[5][5] = {
    /* y=0 */ {0, 1, 190, 28, 91},
    /* y=1 */ {36, 300, 6, 55, 276},
    /* y=2 */ {3, 10, 171, 153, 231},
    /* y=3 */ {105, 45, 15, 21, 136},
    /* y=4 */ {210, 66, 253, 120, 78}};

// rotate-left the with-bits bigint in lane
//
// contract: shift < width
static void rol(uint32_t* out, uint32_t const* in, size_t width, size_t shift)
{
  size_t np = width / 32;
  for (size_t i = 0; i < np; i++) {
    size_t prev_i = (i - 1) % np;
    out[i] = (in[i] << shift) | (in[prev_i] >> (32 - shift));
  }
}

static void rho(uint32_t* out, uint32_t const* in, size_t width)
{
  for (size_t y = 0; y < 5; y++) {
    for (size_t x = 0; x < 5; x++) {
      size_t shift = RHO_STEP_MAPPINGS[y][x];
      rol(&out[part_idx(x, y, 0)], &in[part_idx(x, y, 0)], width,
          shift % width);
    }
  }
}

static void copy_lane(uint32_t* out, uint32_t const* in, size_t width)
{
  memcpy(out, in, 4 * (width / 32));
}

static void copy_state(uint32_t* out, uint32_t const* in, size_t width)
{
  memcpy(out, in, 4 * (width / 32) * 25);
}

static void pi(uint32_t* out, uint32_t const* in, size_t width)
{
  for (size_t y = 0; y < 5; y++) {
    for (size_t x = 0; x < 5; x++) {
      size_t from_x = (x + 3 * y) % 5;
      size_t from_y = x;
      copy_lane(&out[part_idx(x, y, 0)], &in[part_idx(from_x, from_y, 0)],
                width);
    }
  }
}

static void chi(uint32_t* out, uint32_t const* in, size_t width)
{
  for (size_t x = 0; x < 5; x++) {
    size_t right_x = (x + 1) % 5;
    size_t right_right_x = (x + 2) % 5;
    for (size_t y = 0; y < 5; y++) {
      for (size_t zpart = 0; zpart < width / 32; zpart++) {
        uint32_t current = in[part_idx(x, y, zpart)];
        uint32_t right = in[part_idx(right_x, y, zpart)];
        uint32_t right_right = in[part_idx(right_right_x, y, zpart)];
        out[part_idx(x, y, zpart)] = current ^ ((right ^ 1U) & right_right);
      }
    }
  }
}

static uint32_t rc(uint32_t x)
{
  return bit_get(SLOWCRYPT_SHA3_RC[x / 32], x % 32);
}

// TODO: optimize
static void iota(uint32_t* out,
                 uint32_t const* in,
                 size_t width,
                 size_t log2width,
                 size_t round)
{
  copy_state(out, in, width);

  for (size_t j = 0; j <= log2width; j++) {
    size_t zbit = 2 * j - 1;
    out[part_idx(0, 0, zbit / 32)] = bit_replace(out[part_idx(0, 0, zbit / 32)],
                                                 zbit % 32, rc(j + 7 * round));
  }

  for (size_t zpart = 0; zpart < width / 32; zpart++) {
    out[part_idx(0, 0, zpart)] =
        in[part_idx(0, 0, zpart)] ^ out[part_idx(0, 0, zpart)];
  }
}

/**
 * Contracts:
 * - 'width % 25 == 0'
 * - `nr <= 24`
 * - 'log2width  == log2(width)'
 * - `log2width  < 7'
 * - 'lenof(out) == width / 8 * 25'
 * - 'lenof(in)  == width / 8 * 25'
 * - 'lenof(state0) == width / 32 * 25'
 * - 'lenof(state1) == width / 32 * 25'
 */
void slowcrypt_keccak_p(size_t width,
                        size_t log2width,
                        size_t nr,
                        uint8_t* out,
                        uint8_t const* in,
                        uint32_t* state0,
                        uint32_t* state1)
{
  read_state(state0, in, width);

  // For ir from 12 + 2l – nr to 12 + 2l – 1, let A = Rnd(A, ir)
  for (size_t ir = 12 + 2 * log2width - nr; ir < 12 + 2 * log2width; ir++) {
    theta(state1, state0, width);
    rho(state0, state1, width);
    pi(state1, state0, width);
    chi(state0, state1, width);
    iota(state1, state0, width, log2width, ir);
    copy_state(state0, state1, width);
  }

  write_state(out, state0, width);
}

static inline void keccak_sponge_f(uint8_t* out,
                                   uint8_t const* in,
                                   uint32_t* state0,
                                   uint32_t* state1)
{
  slowcrypt_keccak_p(1600 / 25, 6, 24, out, in, state0, state1);
}

/**
 * Contracts:
 * - 'x > 0'
 * - 'm >= 0'
 */
static void pad10t1(int32_t x, int32_t m)
{
  int32_t j = (-m - 2) % x;
  // '1 || 0 * j || 1'
}

static uint16_t const algo_cap[] = {
    [SLOWCRYPT_SHA3_224 - SLOWCRYPT_ALGO_FIRST_KECCAK] = 448,
    [SLOWCRYPT_SHA3_256 - SLOWCRYPT_ALGO_FIRST_KECCAK] = 512,
    [SLOWCRYPT_SHA3_384 - SLOWCRYPT_ALGO_FIRST_KECCAK] = 768,
    [SLOWCRYPT_SHA3_512 - SLOWCRYPT_ALGO_FIRST_KECCAK] = 1024,
    [SLOWCRYPT_SHAKE128 - SLOWCRYPT_ALGO_FIRST_KECCAK] = 256,
    [SLOWCRYPT_SHAKE256 - SLOWCRYPT_ALGO_FIRST_KECCAK] = 512,
};

void slowcrypt_keccak_int(slowcrypt_keccak_sponge* sponge, int algo)
{
  memset(sponge, 0, sizeof(*sponge));
  sponge->c = algo_cap[algo - SLOWCRYPT_ALGO_FIRST_KECCAK];
  sponge->r = 1600 - sponge->c;
}

void slowcrypt_keccak_absorb(slowcrypt_keccak_sponge* sponge,
                             uint8_t const* data)
{
  // TODO
}

size_t slowcrypt_keccak_squeeze_chunk_size(
    slowcrypt_keccak_sponge const* sponge)
{
  return sponge->r / 8;
}

void slowcrypt_keccak_squeeze(uint8_t* out, slowcrypt_keccak_sponge* sponge)
{
  // TODO
}

void slowcrypt_keccak_deint(slowcrypt_keccak_sponge* sponge)
{
  volatile uint8_t* p = (void*)sponge;
  for (size_t i = 0; i < sizeof(*sponge); i++)
    p[i] = 0;
}