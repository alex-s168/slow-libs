
/*
 * Copyright (c) 2026 Alexander Nutz
 * 0BSD licensed, see below documentation
 *
 * Latest version can be found at:
 * https://git.vxcc.dev/alexander.nutz/slow-libs
 *
 *
 * ======== Fixed-size big integers ========
 *
 * Similar to _BitInt
 *
 * Security considerations:
 * - timing attacks:
 *   To prevent timing attacks. Make sure you are using a non-optimizing compiler,
 *   AND define SLOWLIBS_FBIG_CONSTANT_TIME.
 *
 *
 * Configuration options:
 * - SLOWLIBS_FBIG_CONSTANT_TIME:
 *     If defined, does not use clearly non-constant time algorithms.
 *     NOTE that this does not guarantee that the implementation is actually constant time, as it depends on the compiler and platform.
 *     Use a non-optimizing compiler to make sure that the implementation is actually constant time. 
 * - slowlib_fbig_part:
 *     The type of the individual parts of the big integer. Must be an unsigned integer type.
 *     The default can NOT be relied on!
 * - slowlib_fbig_double_part:
 *     A unsigned integer at least twice as wide as slowlib_fbig_part, used for intermediate values in multiplication and addition.
 *     The default can NOT be relied on!
 *
 *
 * Compatibility:
 *   requires only a C99 compiler.
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

#ifndef SLOWLIBS_FIXED_BIGINT_H
#define SLOWLIBS_FIXED_BIGINT_H

#include <stddef.h>
#include <stdint.h>

// TODO: there are definitely still some non-constant time algorithms in here, need to review and fix those
// TODO: put some impls into functions

#ifndef slowlib_fbig_part
// biggest is on the right
typedef uint32_t slowlib_fbig_part;
typedef uint64_t slowlib_fbig_double_part;
#endif

#define slowlib_fbig_const_part_u64(u64) \
  ((uint64_t)(u64) & 0xFFFFFFFF), (((uint64_t)(u64) >> 32) & 0xFFFFFFFF)

#define slowlib_fbig(bitwidth) (((bitwidth) + 31) / 32)

/**
 * Example:
 * ```c
 * slowlib_fbig_const(136, P,
 *                   {slowlib_fbig_const_part_u64(0xfffffffffffffffb),
 *                    slowlib_fbig_const_part_u64(0xffffffffffffffff),
 *                    slowlib_fbig_const_part_u64(0x3)});
 * ```
 */
#define slowlib_fbig_const(width, name, ...)           \
  /* wasting bytes to not get excess element errors */ \
  static slowlib_fbig_part const                       \
      name[slowlib_fbig(((width) + 63) / 64 * 64)] = __VA_ARGS__

#define slowlib_fbig_var(width, name) \
  slowlib_fbig_part name[slowlib_fbig((width))]

#define slowlib_fbig_np(arr) (sizeof(arr) / sizeof(slowlib_fbig_part))
#define slowlib_fbig_part_bits (sizeof(slowlib_fbig_part) * 8)
#define slowlib_fbig_part_sz (sizeof(slowlib_fbig_part))
#define slowlib_fbig_nb(arr) \
  (sizeof(arr) / sizeof(slowlib_fbig_part) * slowlib_fbig_part_bits)

// biggest is on the right (little endian)
#define slowlib_fbig_as_bytes(arr) ((uint8_t*)(slowlib_fbig_part*)(arr))

#define slowlib_fbig_static_assert(cond, msg)                          \
  do {                                                                 \
    typedef char slowlib_fbig_static_assertion_##msg[(cond) ? 1 : -1]; \
    slowlib_fbig_static_assertion_##msg _slowlib_fbig_assertion;       \
    (void)_slowlib_fbig_assertion;                                     \
  } while (0)

typedef char slowlib_fbig__static_assertion__part_sizes
    [(sizeof(slowlib_fbig_double_part) >= 2 * sizeof(slowlib_fbig_part)) ? 1
                                                                         : -1];

#define slowlib_fbig_zext(out, src)                                            \
  do {                                                                         \
    slowlib_fbig_part* _outp = (out);                                          \
    slowlib_fbig_part const* _srcp = (src);                                    \
    slowlib_fbig_static_assert(sizeof((out)) >= sizeof((src)), out_too_small); \
    for (size_t _i = 0; _i < slowlib_fbig_np((out)); _i++) {                   \
      _outp[_i] = (_i * slowlib_fbig_part_sz < sizeof((src))) ? _srcp[_i] : 0; \
    }                                                                          \
  } while (0)

#define slowlib_fbig_trunc(out, src)                                           \
  do {                                                                         \
    slowlib_fbig_part* _outp = (out);                                          \
    slowlib_fbig_part const* _srcp = (src);                                    \
    slowlib_fbig_static_assert(sizeof((out)) <= sizeof((src)), out_too_large); \
    for (size_t _i = 0; _i < slowlib_fbig_np((out)); _i++) {                   \
      _outp[_i] = _srcp[_i];                                                   \
    }                                                                          \
  } while (0)

#define slowlib_fbig_write(outp, src)                        \
  do {                                                       \
    slowlib_fbig_part* _outp = (outp);                       \
    slowlib_fbig_part const* _srcp = (src);                  \
    for (size_t _i = 0; _i < slowlib_fbig_np((src)); _i++) { \
      *_outp++ = _srcp[_i];                                  \
    }                                                        \
  } while (0)

#define slowlib_fbig_add(out, a, b)                                         \
  do {                                                                      \
    slowlib_fbig_part* _outp = (out);                                       \
    slowlib_fbig_part const* _ap = (a);                                     \
    slowlib_fbig_part const* _bp = (b);                                     \
    slowlib_fbig_static_assert(                                             \
        sizeof((out)) >= sizeof((a)) && sizeof((out)) >= sizeof((b)),       \
        out_too_small);                                                     \
    slowlib_fbig_part _carry = 0;                                           \
    for (size_t _i = 0; _i < slowlib_fbig_np((out)); _i++) {                \
      slowlib_fbig_part _a_val = (_i < slowlib_fbig_np((a))) ? _ap[_i] : 0; \
      slowlib_fbig_part _b_val = (_i < slowlib_fbig_np((b))) ? _bp[_i] : 0; \
      slowlib_fbig_double_part _sum = (slowlib_fbig_double_part)_a_val +    \
                                      (slowlib_fbig_double_part)_b_val +    \
                                      (slowlib_fbig_double_part)_carry;     \
      _outp[_i] = (slowlib_fbig_part)_sum;                                  \
      _carry = (slowlib_fbig_part)(_sum >> slowlib_fbig_part_bits);         \
    }                                                                       \
  } while (0)

#define slowlib_fbig_mul(out, a, b)                                           \
  do {                                                                        \
    slowlib_fbig_part* _outp = (out);                                         \
    slowlib_fbig_part const* _ap = (a);                                       \
    slowlib_fbig_part const* _bp = (b);                                       \
    slowlib_fbig_static_assert(sizeof((out)) >= sizeof((a)) + sizeof((b)),    \
                               out_too_small);                                \
    /* FIX: Declare array directly using the part count */                    \
    slowlib_fbig_part _temp[sizeof((out)) / sizeof(slowlib_fbig_part)] = {0}; \
    for (size_t _i = 0; _i < slowlib_fbig_np((a)); _i++) {                    \
      slowlib_fbig_part _carry = 0;                                           \
      for (size_t _j = 0; _j < slowlib_fbig_np((b)); _j++) {                  \
        slowlib_fbig_double_part _prod =                                      \
            (slowlib_fbig_double_part)(_ap[_i]) *                             \
                (slowlib_fbig_double_part)_bp[_j] +                           \
            _temp[_i + _j] + _carry;                                          \
        _temp[_i + _j] = (slowlib_fbig_part)_prod;                            \
        _carry = (slowlib_fbig_part)(_prod >> slowlib_fbig_part_bits);        \
      }                                                                       \
      _temp[_i + slowlib_fbig_np((b))] += _carry;                             \
    }                                                                         \
    for (size_t _i = 0; _i < slowlib_fbig_np((out)); _i++) {                  \
      _outp[_i] = _temp[_i];                                                  \
    }                                                                         \
  } while (0)

#define slowlib_fbig_umod(out, a, m)                                          \
  do {                                                                        \
    slowlib_fbig_part* _outp = (out);                                         \
    slowlib_fbig_part const* _ap = (a);                                       \
    slowlib_fbig_part const* _mp = (m);                                       \
    size_t _np_a = slowlib_fbig_np((a));                                      \
    size_t _np_m = slowlib_fbig_np((m));                                      \
    size_t _np_out = slowlib_fbig_np((out));                                  \
    slowlib_fbig_static_assert(                                               \
        sizeof((out)) >= sizeof((a)) && sizeof((out)) >= sizeof((m)),         \
        out_too_small);                                                       \
    /* prevent Modulo by Zero Infinite Loop */                                \
    int _m_is_zero = 1;                                                       \
    for (size_t _i = 0; _i < _np_m; _i++) {                                   \
      if (_mp[_i] != 0) {                                                     \
        _m_is_zero = 0;                                                       \
        break;                                                                \
      }                                                                       \
    }                                                                         \
    if (_m_is_zero) {                                                         \
      volatile int _trap = 0;                                                 \
      _trap = 1 / _trap;                                                      \
    }                                                                         \
    /* long div */                                                            \
    slowlib_fbig_var(slowlib_fbig_nb((a)), _rem) = {0};                       \
    for (size_t _bit_idx = _np_a * slowlib_fbig_part_bits; _bit_idx-- > 0;) { \
      /* Shift _rem left by 1 */                                              \
      slowlib_fbig_part _carry = 0;                                           \
      for (size_t _j = 0; _j < _np_a; _j++) {                                 \
        slowlib_fbig_part _next_carry =                                       \
            _rem[_j] >> (slowlib_fbig_part_bits - 1);                         \
        _rem[_j] = (_rem[_j] << 1) | _carry;                                  \
        _carry = _next_carry;                                                 \
      }                                                                       \
      /* Extract current bit from 'a' and put it in _rem */                   \
      size_t _part_idx = _bit_idx / slowlib_fbig_part_bits;                   \
      size_t _bit_offset = _bit_idx % slowlib_fbig_part_bits;                 \
      _rem[0] |= (_ap[_part_idx] >> _bit_offset) & 1;                         \
      /* Compare _rem and m */                                                \
      int _cmp = 0;                                                           \
      for (size_t _j = _np_a; _j-- > 0;) {                                    \
        slowlib_fbig_part _m_val = (_j < _np_m) ? _mp[_j] : 0;                \
        if (_rem[_j] < _m_val) {                                              \
          _cmp = -1;                                                          \
          break;                                                              \
        } else if (_rem[_j] > _m_val) {                                       \
          _cmp = 1;                                                           \
          break;                                                              \
        }                                                                     \
      }                                                                       \
      /* If _rem >= m, subtract m from _rem */                                \
      if (_cmp >= 0) {                                                        \
        slowlib_fbig_part _borrow = 0;                                        \
        for (size_t _j = 0; _j < _np_a; _j++) {                               \
          slowlib_fbig_part _m_val = (_j < _np_m) ? _mp[_j] : 0;              \
          slowlib_fbig_double_part _diff =                                    \
              (slowlib_fbig_double_part)_rem[_j] - _m_val - _borrow;          \
          _rem[_j] = (slowlib_fbig_part)_diff;                                \
          _borrow = (_diff >> 63) & 1;                                        \
        }                                                                     \
      }                                                                       \
    }                                                                         \
    /* Write result to out */                                                 \
    for (size_t _i = 0; _i < _np_out; _i++) {                                 \
      _outp[_i] = (_i < _np_a) ? _rem[_i] : 0;                                \
    }                                                                         \
  } while (0)

// constant time shl
#define slowlib_fbig_shl(out, a, shift)                                        \
  do {                                                                         \
    slowlib_fbig_part* _outp = (out);                                          \
    slowlib_fbig_part const* _ap = (a);                                        \
    slowlib_fbig_static_assert(sizeof((out)) >= sizeof((a)), out_too_small);   \
    size_t _word_shift = (shift) / slowlib_fbig_part_bits;                     \
    size_t _bit_shift = (shift) % slowlib_fbig_part_bits;                      \
    for (size_t _i = slowlib_fbig_np((out)); _i-- > 0;) {                      \
      slowlib_fbig_part _part =                                                \
          (slowlib_fbig_part)((_i >= _word_shift &&                            \
                               (_i - _word_shift) < slowlib_fbig_np((a)))      \
                                  ? _ap[_i - _word_shift]                      \
                                  : 0);                                        \
      if (_bit_shift != 0) {                                                   \
        slowlib_fbig_part _next_part =                                         \
            (slowlib_fbig_part)((_i > _word_shift && (_i - _word_shift - 1) <  \
                                                         slowlib_fbig_np((a))) \
                                    ? _ap[_i - _word_shift - 1]                \
                                    : 0);                                      \
        _part <<= _bit_shift;                                                  \
        _part |= _next_part >> (slowlib_fbig_part_bits - _bit_shift);          \
      } else {                                                                 \
        _part <<= _bit_shift;                                                  \
      }                                                                        \
      _outp[_i] = (slowlib_fbig_part)_part;                                    \
    }                                                                          \
  } while (0)

#define slowlib_fbig_or(out, a, b)                                          \
  do {                                                                      \
    slowlib_fbig_part* _outp = (out);                                       \
    slowlib_fbig_part const* _ap = (a);                                     \
    slowlib_fbig_part const* _bp = (b);                                     \
    slowlib_fbig_static_assert(                                             \
        sizeof((out)) >= sizeof((a)) && sizeof((out)) >= sizeof((b)),       \
        out_too_small);                                                     \
    for (size_t _i = 0; _i < slowlib_fbig_np((out)); _i++) {                \
      slowlib_fbig_part _a_val = (_i < slowlib_fbig_np((a))) ? _ap[_i] : 0; \
      slowlib_fbig_part _b_val = (_i < slowlib_fbig_np((b))) ? _bp[_i] : 0; \
      _outp[_i] = _a_val | _b_val;                                          \
    }                                                                       \
  } while (0)

// from integer to bigint, zero-extended
#define slowlib_fbig_zext_scalar(out, a)                                     \
  do {                                                                       \
    slowlib_fbig_part* _outp = (out);                                        \
    slowlib_fbig_static_assert(sizeof((out)) >= sizeof((a)), out_too_small); \
    uintmax_t _temp_val = (uintmax_t)(a);                                    \
    for (size_t _i = 0; _i < slowlib_fbig_np((out)); _i++) {                 \
      _outp[_i] = (slowlib_fbig_part)_temp_val;                              \
      /* Shift in two steps to prevent warnings if part_bits == width */     \
      _temp_val = (_temp_val >> (slowlib_fbig_part_bits - 1)) >> 1;          \
    }                                                                        \
  } while (0)

#endif
