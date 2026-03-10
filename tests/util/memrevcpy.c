#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "slowlibs/util.h"

static void assert_mem_eq__impl(void const* a,
                                char const* alabel,
                                void const* b,
                                char const* blabel,
                                size_t len)
{
  uint8_t const* ap = a;
  uint8_t const* bp = b;
  for (size_t i = 0; i < len; i++, ap++, bp++) {
    if (*ap != *bp) {
      fprintf(stderr, "\n");
      fprintf(stderr, "Assertion failed!\n");
      fprintf(stderr, "  assert_mem_eq(%s, %s, %zu)\n\n", alabel, blabel, len);
      fprintf(stderr, "At position %zu\n", i);
      fprintf(stderr, "  Expected: 0x%02hhx\n", *ap);
      fprintf(stderr, "  Got:      0x%02hhx\n", *bp);
      fflush(stdout);
      exit(1);
    }
  }
}

#define assert_mem_eq(got, expect, len) \
  assert_mem_eq__impl(got, #got, expect, #expect, len)

static void test1()
{
  static const char src[] = "abcdefghijkl";
  static char dst[12] = {0};
  slowlibs_memrevcpy(dst, src, 12);
  assert_mem_eq("lkjihgfedcba", dst, 12);
}

static void test2()
{
  static const char src[] = "abcdefghijk";
  static char dst[11] = {0};
  slowlibs_memrevcpy(dst, src, 11);
  assert_mem_eq("kjihgfedcba", dst, 11);
}

static void test3()
{
  static const char src[] = "abcdefghij";
  static char dst[10] = {0};
  slowlibs_memrevcpy(dst, src, 10);
  assert_mem_eq("jihgfedcba", dst, 10);
}

static void test4()
{
  static const char src[] = "a";
  static char dst[1] = {0};
  slowlibs_memrevcpy(dst, src, 1);
  assert_mem_eq("a", dst, 1);
}

static void test5()
{
  static const char src[0];
  static char dst[0];
  slowlibs_memrevcpy(dst, src, 1);
}

static void test_inplace1()
{
  static char src[] = "abcdefghijkl";
  slowlibs_memrevcpy(src, src, 12);
  assert_mem_eq("lkjihgfedcba", src, 12);
}

static void test_inplace2()
{
  static char src[] = "abcdefghijk";
  slowlibs_memrevcpy(src, src, 11);
  assert_mem_eq("kjihgfedcba", src, 11);
}

static void test_inplace3()
{
  static char src[] = "abcdefghij";
  slowlibs_memrevcpy(src, src, 10);
  assert_mem_eq("jihgfedcba", src, 10);
}

static void test_inplace4()
{
  static char src[] = "a";
  slowlibs_memrevcpy(src, src, 1);
  assert_mem_eq("a", src, 1);
}

static void test_inplace5()
{
  static char src[0];
  slowlibs_memrevcpy(src, src, 1);
}

int main()
{
  test1();
  test2();
  test3();
  test4();
  test5();

  test_inplace1();
  test_inplace2();
  test_inplace3();
  test_inplace4();
  test_inplace5();
}
