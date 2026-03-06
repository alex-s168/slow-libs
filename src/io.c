#include "slowlibs/io.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

slowlibs_io_status slowlibs_io_fixed_buf_writer__write(void* ctxp,
                                                       uint8_t const* data,
                                                       size_t data_len)
{
  slowlibs_buf_cursor* ctx = ctxp;
  if (ctx->pos + data_len > ctx->buflen)
    return SLOWLIBS_IO_WRITE_END;
  size_t i;
  for (i = 0; i < data_len; i++)
    ctx->buf[ctx->pos + i] = data[i];
  ctx->pos += data_len;
  return SLOWLIBS_IO_OK;
}

slowlibs_io_status slowlibs_io_fixed_buf_reader__read(size_t* len_out,
                                                      void* ctxp,
                                                      uint8_t* buf,
                                                      size_t read_max)
{
  slowlibs_buf_cursor* ctx = ctxp;
  size_t to_read = read_max;
  if (ctx->pos + to_read > ctx->buflen)
    to_read = ctx->buflen - ctx->pos;
  size_t i;
  for (i = 0; i < to_read; i++)
    buf[i] = ctx->buf[ctx->pos + i];
  ctx->pos += to_read;
  *len_out = to_read;
  return SLOWLIBS_IO_OK;
}

typedef struct
{
  slowlibs_reader backing;
  int owned;
  size_t chunksize;

  size_t cursor;
  size_t buflen;

  uint8_t buf[];
} bufreader_state;

slowlibs_io_status buffered_reader_read(size_t* len_out,
                                        void* ctx,
                                        uint8_t* buf,
                                        size_t read_max)
{
  bufreader_state* state = ctx;

  /* case 1: still data in buf */
  if (state->cursor < state->buflen) {
    size_t const rem = state->buflen - state->cursor;
    if (read_max > rem)
      read_max = rem;
    memcpy(buf, state->buf + state->cursor, read_max);
    *len_out = read_max;
    state->cursor += read_max;
    return SLOWLIBS_IO_OK;
  }

  /* case 2: read exactly chunk size */
  if (state->chunksize == read_max)
    return slowlibs_read(len_out, state->backing, buf, read_max);

  if (read_max > state->chunksize)
    read_max = state->chunksize;

  /* case 3: need put new data into buffer */
  *len_out = 0;

  size_t numread;
  slowlibs_io_status status =
      slowlibs_read(&numread, state->backing, state->buf, state->chunksize);
  if (status != SLOWLIBS_IO_OK && status != SLOWLIBS_IO_READ_END &&
      status != SLOWLIBS_IO_ONLY_CHUNKED)
    return status;

  state->cursor = 0;
  state->buflen = numread;

  if (numread > 0) {
    if (read_max > numread)
      read_max = numread;
    memcpy(buf, state->buf, read_max);
    *len_out = read_max;
    state->cursor = read_max;
    return SLOWLIBS_IO_OK;
  }

  return status;
}

void buffered_reader_close(void* ctx)
{
  bufreader_state* state = ctx;
  if (state->owned)
    slowlibs_close(state->backing);
  free(state);
}

static int make_buffered_reader(slowlibs_reader* out,
                                int owned,
                                slowlibs_reader backing,
                                size_t buflen)
{
  if (!backing.read || !out)
    return 1;

  bufreader_state* state = malloc(buflen + sizeof(bufreader_state));
  if (!state)
    return 1;
  state->backing = backing;
  state->owned = owned;
  state->chunksize = buflen;
  state->cursor = 0;
  state->buflen = 0;
  *out = (slowlibs_reader){
      .ctx = state,
      .close = buffered_reader_close,
      .read = buffered_reader_read,
      .recommended_chunk_size = buflen,
  };
  return 0;
}

int slowlibs_buffered_reader_borrow(slowlibs_reader* out,
                                    slowlibs_reader backing,
                                    size_t buflen)
{
  return make_buffered_reader(out, 0, backing, buflen);
}

int slowlibs_buffered_reader_owned(slowlibs_reader* out,
                                   slowlibs_reader backing,
                                   size_t buflen)
{
  return make_buffered_reader(out, 1, backing, buflen);
}

slowlibs_io_status slowlibs_transfer(slowlibs_writer out,
                                     slowlibs_reader in,
                                     uint8_t* buf,
                                     size_t buflen,
                                     size_t* int_unwritten)
{
  if (buflen == 0)
    return SLOWLIBS_IO_BUFLEN_ZERO;

  if (*int_unwritten) {
    slowlibs_io_status wres = slowlibs_write(out, buf, *int_unwritten);
    if (wres == SLOWLIBS_IO_OK) {
      *int_unwritten = 0;
    } else {
      return wres;
    }
    return SLOWLIBS_IO_YIELD;
  }

  size_t read_len;
  slowlibs_io_status rres = slowlibs_read(&read_len, in, buf, buflen);
  if (rres == SLOWLIBS_IO_READ_END) {
    if (read_len == 0)
      return SLOWLIBS_IO_READ_END;
    rres = SLOWLIBS_IO_OK;
  }
  if (rres != SLOWLIBS_IO_OK)
    return rres;

  *int_unwritten = read_len;
  return SLOWLIBS_IO_YIELD;
}

slowlibs_io_status slowlibs_transfer_noyield(slowlibs_writer out,
                                             slowlibs_reader in,
                                             uint8_t* buf,
                                             size_t buflen,
                                             size_t* int_unwritten)
{
  slowlibs_io_status status;
  do {
    status = slowlibs_transfer(out, in, buf, buflen, int_unwritten);
  } while (status == SLOWLIBS_IO_YIELD);
  return status;
}

static inline size_t ceil_pow2_sz(size_t v)
{
  v--;
  for (size_t i = 1; i < sizeof(size_t) * 8; i <<= 1)
    v |= v >> i;
  return v + 1;
}

static inline size_t gcd_sz(size_t a, size_t b)
{
  while (b) {
    a %= b;
    size_t temp = a;
    a = b;
    b = temp;
  }
  return a;
}

static inline size_t lcm_sz(size_t a, size_t b)
{
  if (a == 0 || b == 0)
    return 0;
  return (a / gcd_sz(a, b)) * b;
}

#define KiB *1024
#define MiB *1024 KiB

size_t slowlibs_transfer_chunk_size(slowlibs_writer out, slowlibs_reader in)
{
  size_t recom = ceil_pow2_sz(
      lcm_sz(out.recommended_chunk_size, in.recommended_chunk_size));
  if (recom == 0)
    recom = 4 KiB;
  if (recom > 64 MiB)
    recom = 64 MiB;
  return recom;
}
