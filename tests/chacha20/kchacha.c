
#include <stdio.h>
#include "slowlibs/chacha20.h"

static char const data[] =
    "DoNotCurrently-Use-KChaCha-InSensitive-Applications!!NeedingMoreBytes-for-"
    "getting-to-three-blocks.";

static uint8_t const protocol_constant[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                                            0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
                                            0x0d, 0x0e, 0x0f, 0xfa};

static uint8_t const expected[] = {
    0x09, 0xff, 0x91, 0x83, 0x19, 0xfa, 0x21, 0xfd, 0x2d, 0x49, 0x13,
    0xf2, 0x7e, 0x00, 0x3e, 0x63, 0x96, 0xf5, 0x69, 0xc4, 0xfc, 0x94,
    0x53, 0xff, 0x31, 0x43, 0x9b, 0x6c, 0xc1, 0x45, 0x22, 0x79,
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
