#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SLOWCRYPT_CHACHA20_IMPL
#include "../chacha20.h"

#define SLOWCRYPT_POLY1305_IMPL
#include "../poly1305.h"

struct algo
{
  char const* name;
  void (*run)(char**);
};

static FILE* file_open(char const* path)
{
  FILE* fp;

  if (!strcmp(path, "-"))
    return stdin;

  fp = fopen(path, "rb");
  if (!fp) {
    fprintf(stderr, "Could not open %s\n", path);
    exit(1);
  }
  return fp;
}

static void file_close(FILE* p)
{
  if (!p)
    return;
  if (p == stdout || p == stdin || p == stderr)
    return;
  fclose(p);
}

static int anyeq__impl(char const* str, char const** opts)
{
  for (; *opts; opts++)
    if (!strcmp(str, *opts))
      return 1;
  return 0;
}
#define anyeq(str, ...) anyeq__impl(str, (char const*[]){__VA_ARGS__, 0})

static char const* parse_hex_prefix(char const* msg)
{
  if (*msg == 'h')
    msg++;
  else if (msg[0] == '0' && msg[1] == 'x')
    msg++;

  return msg;
}

static uint8_t parse_hex_nibble(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 0xA;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 0xA;
  fprintf(stderr, "Not a hexadecimal number!\n");
  exit(1);
}

static uint8_t parse_hex(char const** msg)
{
  uint8_t res = parse_hex_nibble(*(*msg)++);
  if (**msg) {
    res <<= 4;
    res |= parse_hex_nibble(*(*msg)++);
  }
  return res;
}

static void parse_hex2buf(uint8_t* buf,
                          unsigned int buflen,
                          char const* label,
                          char const* hex)
{
  unsigned int num = 0;
  hex = parse_hex_prefix(hex);
  for (; num < buflen && *hex; num++)
    buf[num] = parse_hex(&hex);
  if (num != buflen || *hex) {
    fprintf(stderr, "Expected %s to be %u (hexadecimal) bytes!\n", label,
            buflen);
    exit(1);
  }
}

static unsigned long file_read_chunk(FILE* file,
                                     uint8_t* buf,
                                     unsigned long buflen)
{
  unsigned long n;
  if (feof(file))
    return 0;
  n = fread(buf, 1, buflen, file);
  if (ferror(file)) {
    fprintf(stderr, "File read error!");
    exit(1);
  }
  return n;
}

static void run_chacha20_core(char** args)
{
  static char const help[] =
      "chacha20-core <key> <counter> <nonce>\n"
      "\n"
      "Run the ChaCha20 block function\n";
  char const *key, *nonce;
  unsigned int npos = 0;
  unsigned int nb;
  unsigned long lu;
  uint32_t counter;
  slowcrypt_chacha20 state[2];
  uint8_t buf[64];
  uint8_t keyb[32];
  uint8_t nonceb[12];

  if (!*args) {
    printf("%s", help);
    exit(0);
  }

  for (; *args; args++) {
    if (anyeq(*args, "-h", "-help", "--help")) {
      printf("%s", help);
      exit(0);
    } else if (npos == 2 && ++npos) {
      nonce = *args;
    } else if (npos == 1 && ++npos) {
      sscanf(*args, "%lu", &lu);
      counter = lu;
    } else if (npos == 0 && ++npos) {
      key = *args;
    } else {
      fprintf(stderr, "Unexpected argument: %s\n", *args);
      exit(1);
    }
  }

  if (npos != 3) {
    fprintf(stderr, "Missing arguments!\n");
    exit(1);
  }

  parse_hex2buf(keyb, 32, "key", key);
  parse_hex2buf(nonceb, 12, "nonce", nonce);

  slowcrypt_chacha20_init(state, keyb, counter, nonceb);
  slowcrypt_chacha20_run(state, &state[1], 20);
  slowcrypt_chacha20_serialize(buf, state);

  for (nb = 0; nb < 64; nb++)
    printf("%02x", buf[nb]);
  printf("\n");
}

static void run_poly1305(char** args)
{
  static char const help[] =
      "poly1305 [--key] <hex-key> [file]\n"
      "\n"
      "Run the Poly1305 one-time authenticator on the data from the given file "
      "or stdin\n";
  char const* key = 0;
  char const* fpath = "-";
  FILE* fp;
  unsigned int npos = 0;
  uint8_t keybuf[32];
  uint8_t chunk[16];
  slowcrypt_poly1305 poly1305;
  unsigned int nb;

  if (!*args) {
    printf("%s", help);
    exit(0);
  }

  for (; *args; args++) {
    if (!key && anyeq(*args, "-k", "-key", "--key") && args[1]) {
      args++;
      key = *args;
    } else if (anyeq(*args, "-h", "-help", "--help")) {
      printf("%s", help);
      exit(0);
    } else if (npos == 1 && ++npos) {
      fpath = *args;
    } else if (npos == 0 && ++npos && !key) {
      key = *args;
    } else {
      fprintf(stderr, "Unexpected argument: %s\n", *args);
      exit(1);
    }
  }

  if (!key) {
    fprintf(stderr, "Missing argument: [--key] <hex-key>");
    exit(1);
  }

  fp = file_open(fpath);

  parse_hex2buf(keybuf, 32, "key", key);
  slowcrypt_poly1305_init(&poly1305, keybuf);

  while ((nb = file_read_chunk(fp, chunk, 16))) {
    slowcrypt_poly1305_next_block(&poly1305, chunk, nb);
  }
  slowcrypt_poly1305_finish(&poly1305, chunk);

  for (nb = 0; nb < 16; nb++)
    printf("%02x", chunk[nb]);
  printf("\n");

  file_close(fp);
}

static struct algo bytes2sum[] = {{"poly1305", run_poly1305},
                                  {"chacha20-core", run_chacha20_core},
                                  {0, 0}};
static struct algo bytes2bytes[] = {  //{"chacha20", run_chacha20_crypt},
    {0, 0}};

int main(int argc, char** argv)
{
  argv++;
  struct algo* a;

  if (!*argv || anyeq(*argv, "-h", "-help", "--help")) {
    printf("bytes -> scalar\n");
    for (a = bytes2sum; a->name; a++)
      printf("  %s\n", a->name);
    printf("\nbytes -> bytes\n");
    for (a = bytes2bytes; a->name; a++)
      printf("  %s\n", a->name);
    return 0;
  }

  for (a = bytes2sum; a->name; a++) {
    if (!strcmp(a->name, *argv)) {
      a->run(argv + 1);
      return 0;
    }
  }

  for (a = bytes2bytes; a->name; a++) {
    if (!strcmp(a->name, *argv)) {
      a->run(argv + 1);
      return 0;
    }
  }

  fprintf(stderr, "Unknown algorithm %s\n", *argv);
  return 1;
}
