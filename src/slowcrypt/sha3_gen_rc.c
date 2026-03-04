
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdint.h>

#define bit_get(x, bit) (((x) >> (bit)) & 1U)
#define bit_replace(x, bit, val) ((x) ^ ((-(val) ^ (x)) & (1U << (bit))))

// standard SHA3 only goes up to 24 rounds, so this is only up to 177
static uint32_t rc(size_t round)
{
  if (round % 255 == 0) {
    return 1;
  }
  uint32_t R = 0x80;
  for (size_t i = 1; i <= round % 255; i++) {
    R <<= 1;
    R = bit_replace(R, 0, bit_get(R, 0) ^ bit_get(R, 8));
    R = bit_replace(R, 4, bit_get(R, 4) ^ bit_get(R, 8));
    R = bit_replace(R, 5, bit_get(R, 5) ^ bit_get(R, 8));
    R = bit_replace(R, 6, bit_get(R, 6) ^ bit_get(R, 8));
    R &= 0xFF;
  }
  return R & 1;
}

static char const HEX[16] = "0123456789abcdef";

static void write_hex_byte(FILE* out, uint8_t by)
{
  char c[2];
  c[0] = HEX[(by >> 4) & 0xF];
  c[1] = HEX[by & 0xF];
  fwrite(c, 1, 2, out);
}

static void write_hex_u32(FILE* out, uint32_t u)
{
  size_t shift = 32;
  while (shift > 0) {
    shift -= 8;
    write_hex_byte(out, (u >> shift) & 0xF);
  }
}

int main()
{
  FILE* out = stdout;

  fprintf(out, "static uint32_t const SLOWCRYPT_SHA3_RC[] = {\n  ");
  uint32_t part = 0;
  for (size_t i = 0; i <= 177; i++) {
    uint32_t bit = rc(i);
    part = bit_replace(part, (i % 8), bit);
    if ((i + 1) % 8 == 0) {
        fprintf(out, "0x");
        write_hex_u32(out, part);
        fprintf(out, ", ");
    }
  }
  fprintf(out, "\n};\n");
}