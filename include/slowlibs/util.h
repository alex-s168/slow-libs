#ifndef SLOWLIBS_UTIL_H
#define SLOWLIBS_UTIL_H

#include <limits.h>

#ifdef BITINT_MAXWIDTH
#if BITINT_MAXWIDTH >= 264
#define SLOWLIBS_BITINT_MAXWIDTH BITINT_MAXWIDTH
#endif
#elif defined(__BITINT_MAXWIDTH__)
#if __BITINT_MAXWIDTH__ >= 264
#define SLOWLIBS_BITINT_MAXWIDTH __BITINT_MAXWIDTH__
#endif
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define slowlibs_O0 __attribute__((optimize("O0")))
#else
#define slowlibs_O0 /**/
#endif

#endif