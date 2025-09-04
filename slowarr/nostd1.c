void c_memzero(void *ptrin, unsigned len) {
  char *ptr = ptrin;
  for (; len;) {
    *ptr = 0;
    len -= 1;
    ptr += 1;
  }
}

void c_memmove(void *destp, void *srcp, unsigned len) {
  char *dest = destp;
  char *src = srcp;
  for (; len;) {
    *dest = *src;
    len -= 1;
    src += 1;
    dest += 1;
  }
}

#define SLOWARR_ASSERT_USER_ERROR(expr) /**/
#define SLOWARR_MEMZERO(ptr, len) c_memzero(ptr, len)
#define SLOWARR_MEMMOVE(dest, src, ln) c_memmove(dest, src, ln)
#define SLOWARR_SZT unsigned long
#define SLOWARR_REALLOC(ptr, old, news) ((void *)0)
#define SLOWARR_FREE(ptr, size)   /**/
#define SLOWARR_ON_MALLOC_FAIL(x) /**/
#define SLOW_DEFINE_ACCESS
#include "../slowarr.h"

typedef char const *cstr;
SLOWARR_Header(cstr);
SLOWARR_Impl(cstr);

int main(int argc, char const **argv) {
  T(SLOWARR, cstr) arr = F(SLOWARR, cstr, borrow)(argv, argc);
  (void)arr;
  return 0;
}
