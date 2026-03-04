#ifndef SLOWCRYPT_SHA3_H
#define SLOWCRYPT_SHA3_H

#include <stddef.h>
#include <stdint.h>

/**
 * Contracts:
 * - 'r + c == 1600'
 */
typedef struct
{
  uint16_t c, r;
  uint8_t sponge[1600 / 8];

  uint8_t staged[1600 / 8];
  size_t staged_len;

  uint32_t state0[1600 / 32];
  uint32_t state1[1600 / 32];
} slowcrypt_keccak_sponge;

/**
 * Parameters:
 * - algo: slowcrypt_algo
 */
void slowcrypt_keccak_int(slowcrypt_keccak_sponge* sponge, int algo);

void slowcrypt_keccak_absorb(slowcrypt_keccak_sponge* sponge,
                             uint8_t const* data);

size_t slowcrypt_keccak_squeeze_chunk_size(
    slowcrypt_keccak_sponge const* sponge);
void slowcrypt_keccak_squeeze(uint8_t* out, slowcrypt_keccak_sponge* sponge);

void slowcrypt_keccak_deint(slowcrypt_keccak_sponge* sponge);

#endif