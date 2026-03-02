#define SLOWCRYPT_POLY1305_IMPL
#define SLOWCRYPT_ALLOW_TIMING_ATTACKS

#include <limits.h>
#ifndef BITINT_MAXWIDTH
#define BITINT_MAXWIDTH 264 
#endif

#include "slowlibs/poly1305.h"

#ifdef SLOWLIBS_FIXED_BIGINT_H
}}} ERROR ERROR ERROR {{{
#endif

#include "test_vector.h"