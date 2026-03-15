
#include <stdio.h>
#include "slowlibs/chacha20.h"

static char const data[] =
    "DoNotCurrently-Use-KChaCha-InSensitive-Applications!!NeedingMoreBytes-for-"
    "getting-to-three-blocks.";

static uint8_t const protocol_constant[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                                            0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
                                            0x0d, 0x0e, 0x0f, 0xfa};

static uint8_t const expected[] = {
    0xda, 0x0e, 0xb9, 0xe9, 0x8b, 0x48, 0x2a, 0x18, 0x2f, 0xe3, 0xdf,
    0xd3, 0x74, 0x39, 0xa9, 0xdd, 0xc4, 0xb9, 0xad, 0xbe, 0x3f, 0xab,
    0xf8, 0x17, 0xea, 0xd2, 0x25, 0x0f, 0x6c, 0xa1, 0x60, 0x99,
};

int main(int argc, char** argv)
{
  uint8_t hash[32];
  int i;

  (void)argc;
  (void)argv;

  slowcrypt_kchacha(hash, protocol_constant, (void*)data, sizeof(data) - 1, 20);

  // for (i = 0; i < 32; i++)
  //   printf("0x%02x, ", hash[i]);
  // printf("\n");

  for (i = 0; i < 32; i++) {
    if (hash[i] != expected[i])
      return 1;
  }

  return 0;
}
