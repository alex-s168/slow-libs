#ifndef SLOWCRYPT_AED_H
#define SLOWCRYPT_AED_H

#include <stdint.h>
#include <stdio.h>

#include "chacha20.h"
#include "io.h"
#include "poly1305.h"

typedef struct
{
  size_t key_len;
  size_t plaintext_max, associated_data_max;
  size_t nonce_min, nonce_max;
  size_t chipertext_max;

  struct
  {
    /* only returns NULL on failure! */
    void* (*create_ctx)(void);
    void (*destroy_ctx)(void* ctx);

    /* returns 0 on success; may NOT destroy the ctx */
    int (*run)(void* ctx,
               /* read chipertext */
               slowlibs_reader* out,
               uint8_t const* key,
               size_t key_len,
               uint8_t const* nonce,
               size_t nonce_len,
               /* needs to be closed! */
               slowlibs_reader plain,
               /* needs to be closed! */
               slowlibs_reader associated_data);
  } encrypt;

  struct
  {
    /* only returns NULL on failure! */
    void* (*create_ctx)(void);
    void (*destroy_ctx)(void* ctx);

    /* returns 0 on success; may NOT destroy the ctx */
    int (*run)(void* ctx,
               /* read plaintext */
               slowlibs_reader* out,
               uint8_t const* key,
               size_t key_len,
               uint8_t const* nonce,
               size_t nonce_len,
               /* needs to be closed! */
               slowlibs_reader chipertext,
               /* needs to be closed! */
               slowlibs_reader associated_data);
  } decrypt;
} slowcrypt_aed;

// TODO: When the length of P is zero, the AEAD algorithm acts as a Message Authentication Code on the input A.

#ifdef SLOWCRYPT_AED_CHACHA20_POLY1305

#endif

#endif