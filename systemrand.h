/*
 * Copyright (c) 2025 Alexander Nutz
 * MIT licensed, see below documentation
 *
 * Latest version can be found at:
 * https://gitea.vxcc.dev/alexander.nutz/slow-libs
 *
 * ======== High-entropy system RNG =========
 *
 * Configuration options:
 * - SLOWCRYPT_SYSTEMRAND_IMPL
 * - SLOWCRYPT_SYSTEMRAND_FUNC
 *     will be used in front of every function definition / declaration
 *
 * Tries these sources (depending on platform support), in order:
 * - getentropy()
 * - getrandom()
 * - unless _WIN32 is defined, read from /dev/random or /dev/urandom
 * - on _WIN32, try BCryptGenRandom
 * - on _WIN32, CryptGenRandom
 * - on _WIN32, RtlGenRandom
 * - random(), unless bail on insecure
 * - rand(), unless bail on insecure
 *
 * As with all slow-libs, a wide range of supported platforms is a priority,
 * so please report any incompatibility issues.
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

#ifndef SLOWCRYPT_SYSTEMRAND_H
#define SLOWCRYPT_SYSTEMRAND_H

#ifndef SLOWCRYPT_SYSTEMRAND_FUNC
#define SLOWCRYPT_SYSTEMRAND_FUNC /**/
#endif

typedef enum
{
  SLOWCRYPT_SYSTEMRAND__BAIL_IF_INSECURE = 1 << 0,
  SLOWCRYPT_SYSTEMRAND__INSECURE_NON_BLOCKING = 1 << 1
} slowcrypt_systemrand_flags;

/*
 * Return codes:
 * - 0: success
 * - 1: unexpected error
 * - 2: random number is not secure enough. Only returned with BAIL_IF_INSECURE
 */
SLOWCRYPT_SYSTEMRAND_FUNC int slowcrypt_systemrand(
    void* buffer,
    unsigned long length,
    slowcrypt_systemrand_flags flags);

#ifdef SLOWCRYPT_SYSTEMRAND_IMPL

#ifndef _GNU_SOURCE
#define SLOWCRYPT_SYSTEMRAND_UNDEF_GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef unix
#include <errno.h>
#include <unistd.h>
#endif

#ifdef SLOWCRYPT_SYSTEMRAND_UNDEF_GNU_SOURCE
#undef SLOWCRYPT_SYSTEMRAND_UNDEF_GNU_SOURCE
#undef _GNU_SOURCE
#endif

static int slowcrypt_systemrand__chunk256(void* buffer,
                                          unsigned long length,
                                          slowcrypt_systemrand_flags flags)
{
  unsigned long i, j;
  unsigned char u8;
  char const* fpath;
  FILE* fp;

#if defined(unix) && defined(__GLIBC__) && \
    (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25))

  if (getentropy(buffer, length)) {
    if (errno != ENOSYS) {
      return 1;
    }
  } else {
    return 0;
  }
#endif

  /* TODO: getrandom (on glibc, freebsd, and probably others) */

#ifndef _WIN32
  fpath = (flags & SLOWCRYPT_SYSTEMRAND__INSECURE_NON_BLOCKING) ? "/dev/urandom"
                                                                : "/dev/random";

  fp = fopen(fpath, "rb");
  if (fp) {
    i = fread(buffer, 1, length, fp);
    fclose(fp);
    if (i == length)
      return 0;
  }
#endif

  /* TODO: windows stuff */

  if (flags & SLOWCRYPT_SYSTEMRAND__BAIL_IF_INSECURE)
    return 2;

  /* TODO: random() */

  if (RAND_MAX >= 255) {
    for (i = 0; i < length; i++) {
      ((unsigned char*)buffer)[i] =
          (unsigned char)((unsigned int)rand() / (unsigned int)(RAND_MAX));
    }
  } else {
    /* why would this ever happen... */
    for (i = 0; i < length; i++) {
      u8 = 0;
      for (j = 0; j < 8; j++) {
        u8 <<= 1;
        u8 ^= (unsigned char)rand();
      }
      ((unsigned char*)buffer)[i] = u8;
    }
  }
  return 0;
}

SLOWCRYPT_SYSTEMRAND_FUNC int slowcrypt_systemrand(
    void* buffer,
    unsigned long length,
    slowcrypt_systemrand_flags flags)
{
  unsigned int i, j;
  int rc;

  /* many random sources have a limit of 256 bytes */
  for (i = 0; i < length; i += 256) {
    j = length - i;
    if (j > 256)
      j = 256;
    if ((rc = slowcrypt_systemrand__chunk256(buffer, length, flags))) {
      return rc;
    }
  }
  return 0;
}

#endif

#endif
