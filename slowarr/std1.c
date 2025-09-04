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
