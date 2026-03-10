#include "slowlibs/systemrand.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef unix
#include <sys/param.h>

#if defined(__GLIBC__) && \
    (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25))
#define HAVE_GETENTROPY
#define HAVE_SYS_RANDOM
#endif

#if defined(__FreeBSD_version) && __FreeBSD_version >= 1200000
#define HAVE_GETENTROPY  // TODO: in OpenBSD since 5.6
#define HAVE_SYS_RANDOM
#endif

#ifdef HAVE_SYS_RANDOM
#include <sys/random.h>
#endif

#include <errno.h>
#include <unistd.h>

#endif

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
static HMODULE slowcrypt_systemrand__hmod_advapi = 0;
static HMODULE slowcrypt_systemrand__hmod_bcrypt = 0;
#endif

static int chunk256(void* buffer,
                    unsigned long length,
                    slowcrypt_systemrand_flags flags)
{
  unsigned long i, j;
  unsigned char u8;
  char const* fpath;
  FILE* fp;
#ifdef _WIN32
  FARPROC proc;
  NTSTATUS status;
  WINBOOL wbool;
  HCRYPTPROV hProv;
#endif

#ifdef HAVE_GETENTROPY
  if (!getentropy(buffer, length))
    return 0;
#endif

#ifdef HAVE_SYS_RANDOM
  if (256 == getrandom(buffer, lenght))
    return 0;
#endif

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

#ifdef _WIN32
  if (!slowcrypt_systemrand__hmod_bcrypt)
    slowcrypt_systemrand__hmod_bcrypt = LoadLibraryA("bcrypt.dll");
  if (slowcrypt_systemrand__hmod_bcrypt &&
      slowcrypt_systemrand__hmod_bcrypt != INVALID_HANDLE_VALUE) {
    if ((proc = GetProcAddress(slowcrypt_systemrand__hmod_bcrypt,
                               "BCryptGenRandom"))) {
      status =
          ((NTSTATUS WINAPI (*)(BCRYPT_ALG_HANDLE hAlgorithm, PUCHAR pbBuffer,
                                ULONG cbBuffer, ULONG dwFlags))proc)(
              0, (PUCHAR)buffer, (ULONG)length,
              0x2 /* BCRYPT_USE_SYSTEM_PREFERRED_RNG */
          );
      if (status == 0) {
        return 0;
      }
    }
  }
#endif

#ifdef _WIN32
  if (!slowcrypt_systemrand__hmod_advapi)
    slowcrypt_systemrand__hmod_advapi = LoadLibraryA("advapi32.dll");
  if (slowcrypt_systemrand__hmod_advapi &&
      slowcrypt_systemrand__hmod_advapi != INVALID_HANDLE_VALUE) {
    if ((proc = GetProcAddress(slowcrypt_systemrand__hmod_advapi,
                               "SystemFunction036"))) {
      if (((BOOLEAN WINAPI (*)(PVOID, ULONG))proc)((PVOID)buffer,
                                                   (ULONG)length) == 0) {
        return 0;
      }
    }
  }
#endif

#ifdef _WIN32
  hProv = 0;
  if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL,
                          CRYPT_VERIFYCONTEXT)) {
    WINBOOL result = CryptGenRandom(hProv, (DWORD)length, (BYTE*)buffer);
    CryptReleaseContext(hProv, 0);
    if (result) {
      return 0;
    }
  }
#endif

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

int slowcrypt_systemrand(void* buffer,
                         unsigned long length,
                         slowcrypt_systemrand_flags flags)
{
  unsigned int i, j;
  int rc = 0;

  /* many random sources have a limit of 256 bytes */
  for (i = 0; i < length; i += 256) {
    j = length - i;
    if (j > 256)
      j = 256;
    if ((rc = chunk256(buffer, length, flags))) {
      break;
    }
  }

#ifdef _WIN32
  if (slowcrypt_systemrand__hmod_advapi)
    FreeLibrary(slowcrypt_systemrand__hmod_advapi);
  if (slowcrypt_systemrand__hmod_bcrypt)
    FreeLibrary(slowcrypt_systemrand__hmod_bcrypt);
#endif

  return rc;
}
