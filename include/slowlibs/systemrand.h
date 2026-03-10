/*
 * Copyright (c) 2025 Alexander Nutz
 * 0BSD licensed, see below documentation
 *
 * Latest version can be found at:
 * https://git.vxcc.dev/alexander.nutz/slow-libs
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
 * - on _WIN32, try RtlGenRandom
 * - on _WIN32, try CryptGenRandom
 * - random(), unless bail on insecure
 * - rand(), unless bail on insecure
 *
 * As with all slow-libs, a wide range of supported platforms is a priority,
 * so please report any compatibility issues.
 */

/*
 * Copyright (C) 2026 by Alexander Nutz <alexander.nutz@vxcc.dev>
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 * NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
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

#endif
