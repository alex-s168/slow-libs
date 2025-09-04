#ifndef SLOWURL_H_
#define SLOWURL_H_

#ifndef SLOWARR_H_
Please include and configure slowarr.h first;
#include "slowarr.h"
#endif

#ifndef SLOWURL_NAMESPACE
#define SLOWURL_NAMESPACE(X) slowurl_##X
#endif

SLOWARR_Header(char);

typedef struct {

} SLOWURL_NAMESPACE();

#ifdef SLOWURL_IMPLEMENTATION

#endif

#endif
