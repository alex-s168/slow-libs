
#include <stddef.h>
#include <stdio.h>
#include "slowlibs/poly1305.h"

static uint8_t KEY_MATERIAL[] = {
    0x85, 0xd6, 0xbe, 0x78, 0x57, 0x55, 0x6d, 0x33, 0x7f, 0x44, 0x52,
    0xfe, 0x42, 0xd5, 0x06, 0xa8, 0x01, 0x03, 0x80, 0x8a, 0xfb, 0x0d,
    0xb2, 0xfd, 0x4a, 0xbf, 0xf6, 0xaf, 0x41, 0x49, 0xf5, 0x1b};

static char const MESSAGE[] = "Cryptographic Forum Research Group";
#define MESSAGE_LEN (sizeof(MESSAGE) - 1)

static uint8_t const EXPECTED_TAG[] = {0xa8, 0x06, 0x1d, 0xc1, 0x30, 0x51,
                                       0x36, 0xc6, 0xc2, 0x2b, 0x8b, 0xaf,
                                       0x0c, 0x01, 0x27, 0xa9};

int main()
{
  slowcrypt_poly1305 state;
  slowcrypt_poly1305_init(&state, KEY_MATERIAL);
  // KEY_MATERIAL invalidated!
  for (size_t i = 0; i < MESSAGE_LEN; i += 16) {
    size_t len = MESSAGE_LEN - i;
    if (len > 16)
      len = 16;
    slowcrypt_poly1305_next_block(&state, (uint8_t const*)MESSAGE + i, len);
  }
  uint8_t tag[16];
  slowcrypt_poly1305_finish(&state, tag);

  for (int i = 0; i < 16; i++)
    printf("%02lx:", (unsigned long)tag[i]);
  printf("\n");

  for (size_t i = 0; i < 16; i++)
    if (tag[i] != EXPECTED_TAG[i])
      return 1;
  return 0;
}