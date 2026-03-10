#ifndef SLOWLIBS_CBOR_H
#define SLOWLIBS_CBOR_H

#include <stdbool.h>
#include <stdint.h>
#include "io.h"
#include "slowlibs/util.h"

typedef struct
{
  // may never return would block or yield!
  slowlibs_writer io;
  slowlibs_endian net_order;
} slowcbor_writer;

typedef enum
{
  SLOWCBOR_TYPE_POSITIVE = 0,
  SLOWCBOR_TYPE_NEGATIVE = 1,

  SLOWCBOR_TYPE_BYTES = 2,
  SLOWCBOR_TYPE_TEXT = 3,
  SLOWCBOR_TYPE_LIST = 4,
  SLOWCBOR_TYPE_MAP = 5,

  SLOWCBOR_TYPE_TAG = 6,
  SLOWCBOR_TYPE_SIMPLE = 7,
} slowcbor_type;

enum
{
  SLOWCBOR_SIMPLE_FALSE = 20,
  SLOWCBOR_SIMPLE_TRUE = 21,
  SLOWCBOR_SIMPLE_NULL = 22,
  SLOWCBOR_SIMPLE_UNDEFINED = 23,
};

slowlibs_io_status slowcbor_write_simple(slowcbor_writer writer, uint8_t value);

static inline slowlibs_io_status slowcbor_write_bool(slowcbor_writer writer,
                                                     bool value)
{
  return slowcbor_write_simple(
      writer, value ? SLOWCBOR_SIMPLE_TRUE : SLOWCBOR_SIMPLE_FALSE);
}

// ^^^header

static slowlibs_io_status write_u16(slowcbor_writer writer, uint16_t v)
{
  if (writer.net_order != SLOWLIBS_ENDIAN_HOST)
    slowlibs_swap_end(v);
  return slowlibs_write_v(writer.io, v);
}

static slowlibs_io_status write_u32(slowcbor_writer writer, uint32_t v)
{
  if (writer.net_order != SLOWLIBS_ENDIAN_HOST)
    slowlibs_swap_end(v);
  return slowlibs_write_v(writer.io, v);
}

static slowlibs_io_status write_u64(slowcbor_writer writer, uint64_t v)
{
  if (writer.net_order != SLOWLIBS_ENDIAN_HOST)
    slowlibs_swap_end(v);
  return slowlibs_write_v(writer.io, v);
}

static slowlibs_io_status write_token_header(slowcbor_writer writer,
                                             unsigned major,
                                             unsigned additional)
{
  return slowlibs_write_u8(writer.io, (major << 5) | additional);
}

static slowlibs_io_status write_token_with_arg(slowcbor_writer writer,
                                               unsigned major,
                                               uint64_t arg)
{
  slowlibs_io_status status;
  if (arg < 0 || arg > UINT32_MAX) {
    if ((status = slowlibs_not_async(write_token_header(writer, major, 27))))
      return status;
    if ((status = slowlibs_not_async(write_u64(writer, arg))))
      return status;
  } else if (arg > UINT16_MAX) {
    if ((status = slowlibs_not_async(write_token_header(writer, major, 26))))
      return status;
    if ((status = slowlibs_not_async(write_u32(writer, (uint32_t)arg))))
      return status;
  } else if (arg > UINT8_MAX) {
    if ((status = slowlibs_not_async(write_token_header(writer, major, 25))))
      return status;
    if ((status = slowlibs_not_async(write_u16(writer, (uint16_t)arg))))
      return status;
  } else if (arg > 24) {
    if ((status = slowlibs_not_async(write_token_header(writer, major, 24))))
      return status;
    if ((status =
             slowlibs_not_async(slowlibs_write_u8(writer.io, (uint8_t)arg))))
      return status;
  } else {
    if ((status = slowlibs_not_async(
             write_token_header(writer, major, (unsigned)arg))))
      return status;
  }
  return SLOWLIBS_IO_OK;
}

slowlibs_io_status slowcbor_write_break(slowcbor_writer writer)
{
  return write_token_header(writer, SLOWCBOR_TYPE_SIMPLE, 31);
}

slowlibs_io_status slowcbor_write_indefinite(slowcbor_writer writer,
                                             slowcbor_type type)
{
  return write_token_header(writer, type, 31);
}

slowlibs_io_status slowcbor_write_finite(slowcbor_writer writer,
                                         slowcbor_type type,
                                         uint64_t len)
{
  return write_token_with_arg(writer, type, len);
}

slowlibs_io_status slowcbor_write_tag(slowcbor_writer writer, uint64_t tag)
{
  return write_token_with_arg(writer, SLOWCBOR_TYPE_TAG, tag);
}

slowlibs_io_status slowcbor_write_simple(slowcbor_writer writer, uint8_t value)
{
  if (value < 24) {
    return write_token_header(writer, SLOWCBOR_TYPE_SIMPLE, value);
  } else {
    slowlibs_io_status status;
    if ((status = slowlibs_not_async(
             write_token_header(writer, SLOWCBOR_TYPE_SIMPLE, 24))))
      return status;
    if ((status = slowlibs_not_async(slowlibs_write_u8(writer.io, value))))
      return status;
    return SLOWLIBS_IO_OK;
  }
}

slowlibs_io_status slowcbor_write_uint(slowcbor_writer writer, uint64_t value)
{
  return write_token_with_arg(writer, SLOWCBOR_TYPE_POSITIVE, value);
}

slowlibs_io_status slowcbor_write_sint(slowcbor_writer writer, int64_t value)
{
  if (value < 0) {
    return write_token_with_arg(writer, SLOWCBOR_TYPE_NEGATIVE,
                                (uint64_t)value);
  } else {
    return write_token_with_arg(writer, SLOWCBOR_TYPE_POSITIVE,
                                (uint64_t)value);
  }
}

/*

    public void writeF16(short x) throws IOException {
        writeTokenHeader(7, 25);
        writeShort(x);
    }

    public void writeF32(int x) throws IOException {
        writeTokenHeader(7, 26);
        writeInt(x);
    }

    public void writeF64(long x) throws IOException {
        writeTokenHeader(7, 27);
        writeLong(x);
    }

 */

#endif
