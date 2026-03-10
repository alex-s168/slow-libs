#ifndef SLOWLIBS_IO_H
#define SLOWLIBS_IO_H

#include <stdint.h>
#include <stdio.h>

/**
 * zero:     OK, data processed   
 * positive: no error: end/try-again
 * negative: bad error
 */
typedef enum
{
  /* ==== good ==== */
  SLOWLIBS_IO_OK = 0,
  /** returned when this is the last data returned */
  SLOWLIBS_IO_READ_END,
  /* try again later, no data processed yet */
  SLOWLIBS_IO_WOULD_BLOCK,
  /* you can try again right now, some data processed */
  SLOWLIBS_IO_YIELD,

  /* ==== bad ==== */
  SLOWLIBS_IO_BUFLEN_ZERO = -9999,
  SLOWLIBS_IO_WRITE_END,
  SLOWLIBS_IO_INTERNAL_OOM,
  SLOWLIBS_IO_INTERNAL_ERROR,
  SLOWLIBS_IO_EXTERNAL_ERROR,
  SLOWLIBS_IO_TIMEOUT,
  /* serializer / deserializer can't deal with returned WOULD_BLOCK or YIELD */
  SLOWLIBS_IO_NOT_ASYNC,
} slowlibs_io_status;

static inline slowlibs_io_status slowlibs_not_async(slowlibs_io_status status)
{
  switch (status) {
    case SLOWLIBS_IO_YIELD:
    case SLOWLIBS_IO_WOULD_BLOCK:
      return SLOWLIBS_IO_NOT_ASYNC;

    default:
      return status;
  }
}

static inline slowlibs_io_status slowlibs_io_chain(slowlibs_io_status a,
                                                   slowlibs_io_status b)
{
  if (a < 0)
    return a;
  return b;
}

typedef slowlibs_io_status slowlibs_write_fn(void* ctx,
                                             uint8_t const* data,
                                             size_t data_len);

typedef slowlibs_io_status slowlibs_read_fn(size_t* len_out,
                                            void* ctx,
                                            uint8_t* buf,
                                            size_t read_max);

typedef struct
{
  void* ctx;
  slowlibs_write_fn* write;
  size_t recommended_chunk_size;
  /* optional! */
  void (*flush)(void* ctx);
  /* optional! */
  void (*close)(void* ctx);
} slowlibs_writer;

typedef struct
{
  void* ctx;
  slowlibs_read_fn* read;
  size_t recommended_chunk_size;
  /* optional! */
  void (*close)(void* ctx);
} slowlibs_reader;

#define slowlibs_close(rw)  \
  do {                      \
    if ((rw).close) {       \
      (rw).close((rw).ctx); \
    }                       \
  } while (0)

#define slowlibs_flush(w) \
  do {                    \
    if ((w).flush) {      \
      (w).flush((w).ctx); \
    }                     \
  } while (0)

static inline slowlibs_io_status slowlibs_write(slowlibs_writer writer,
                                                void const* data,
                                                size_t data_len)
{
  return writer.write(writer.ctx, data, data_len);
}

static inline slowlibs_io_status slowlibs_write_u8(slowlibs_writer writer,
                                                   uint8_t v)
{
  return writer.write(writer.ctx, &v, 1);
}

#define slowlibs_write_v(writer, value) \
  slowlibs_write((writer), &(value), sizeof((value)))

static inline slowlibs_io_status slowlibs_read(size_t* len_out,
                                               slowlibs_reader reader,
                                               void* buf,
                                               size_t read_max)
{
  return reader.read(len_out, reader.ctx, buf, read_max);
}

/* ========================= RW Impls =========================== */

typedef struct
{
  uint8_t* buf;
  size_t buflen;
  size_t pos;
} slowlibs_buf_cursor;
extern slowlibs_write_fn slowlibs_io_fixed_buf_writer__write;
extern slowlibs_read_fn slowlibs_io_fixed_buf_reader__read;

/**
 * Example:
 *   uint8_t buf[100];
 *   slowlibs_buf_cursor ctx = { buf, sizeof(buf) };
 *   slowlibs_writer w = slowlibs_fixed_buf_writer(&ctx, buf, sizeof(buf));
 *   slowlibs_write(&w, data, data_len);
 */
#define slowlibs_fixed_buf_writer(ctx, buf, buflen)                       \
  (slowlibs_writer)                                                       \
  {                                                                       \
    .ctx = (slowlibs_buf_cursor*)ctx,                                     \
    .write = slowlibs_io_fixed_buf_writer__write.recommended_chunk_size = \
        buflen                                                            \
  }

/**
 * Example:
 *   uint8_t buf[100];
 *   size_t data_len;
 *   slowlibs_buf_cursor ctx = { buf, sizeof(buf) };
 *   slowlibs_reader r = slowlibs_fixed_buf_reader(&ctx, buf, sizeof(buf));
 *   if (slowlibs_read(&r, &data_len, buf, sizeof(buf)) != SLOWLIBS_IO_OK) {
 *     // handle error
 *   }
 */
#define slowlibs_fixed_buf_reader(ctx, buf, buflen) \
  (slowlibs_reader)                                 \
  {                                                 \
    .ctx = (slowlibs_buf_cursor*)ctx,               \
    .read = slowlibs_io_fixed_buf_reader__read,     \
    .recommended_chunk_size = buflen                \
  }

/**
 * Arguments:
 * - int_unwritten
 *     Points to a integer that persists during the whole transfer process,
 *     and is initialized to zero.
 *
 * Notes:
 * - does not close or flush!
 *
 *
 * This returns:
 * - SLOWLIBS_IO_OK
 *     if the transfer is complete, and all data has been transferred
 * - SLOWLIBS_IO_WOULD_BLOCK
 *     if either the reader or writer returns SLOWLIBS_IO_WOULD_BLOCK.
 *     DO NOT modify `buf`, `buflen`, or `int_unwritten` in this case.
 * - SLOWLIBS_IO_YIELD 
 *     after one chunk has been transferred
 *     DO NOT modify `buf`, `buflen`, or `int_unwritten` in this case.
 * - negative error
 *     if the reader / writer returns any error
 *
 *
 * Example:
 *   // try allocate a most-optimal-chunk-size buffer,
 *   // reducing chunk size as malloc fails
 *   size_t buflen = slowlibs_transfer_chunk_size(out, in);
 *   uint8_t* buf;
 *   for (;;) {
 *     if (!buflen) return SLOWLIBS_IO_INTERNAL_OOM;
 *     buf = malloc(buflen);
 *     if (buf) break;
 *     buflen >>= 1;
 *   }
 *
 *   size_t unwritten = 0; // internal state of slowlibs_transfer()
 *   slowlibs_io_status status;
 *   while (SLOWLIBS_IO_READ_END != ( status = slowlibs_transfer(out, in, buf, buflen, &unwritten) )) {
 *     if (status < 0) { // result code for "bad thing happened"
 *       free(buf);
 *       return status;
 *     }
 *
 *     if (status == SLOWLIBS_IO_WOULD_BLOCK)
 *       usleep(50);
 *   }
 *
 *   free(buf);
 *   return SLOWLIBS_IO_OK;
 *
 *
 * Also see:
 * - slowlibs_transfer_chunk_size()
 * - slowlibs_transfer_noyield()
 */
slowlibs_io_status slowlibs_transfer(slowlibs_writer out,
                                     slowlibs_reader in,
                                     uint8_t* buf,
                                     size_t buflen,
                                     size_t* int_unwritten);

/**
 * Arguments:
 * - int_unwritten
 *     Points to a integer that persists during the whole transfer process,
 *     and is initialized to zero.
 *
 * Notes:
 * - does not close or flush!
 *
 *
 * This returns:
 * - SLOWLIBS_IO_OK
 *     if the transfer is complete, and all data has been transferred
 * - SLOWLIBS_IO_WOULD_BLOCK
 *     if either the reader or writer returns SLOWLIBS_IO_WOULD_BLOCK.
 *     DO NOT modify `buf`, `buflen`, or `int_unwritten` in this case.
 * - negative error
 *     if the reader / writer returns any error
 *
 *
 * Example:
 *   // try allocate a most-optimal-chunk-size buffer,
 *   // reducing chunk size as malloc fails
 *   size_t buflen = slowlibs_transfer_chunk_size(out, in);
 *   uint8_t* buf;
 *   for (;;) {
 *     if (!buflen) return SLOWLIBS_IO_INTERNAL_OOM;
 *     buf = malloc(buflen);
 *     if (buf) break;
 *     buflen >>= 1;
 *   }
 *
 *   size_t unwritten = 0; // internal state of slowlibs_transfer()
 *   slowlibs_io_status status;
 *   while (SLOWLIBS_IO_READ_END != ( status = slowlibs_transfer_noyield(out, in, buf, buflen, &unwritten) )) {
 *     if (status < 0) { // result code for "bad thing happened"
 *       free(buf);
 *       return status;
 *     }
 *     // if we get here, status is always WOULD_BLOCK
 *     usleep(50);
 *   }
 *
 *   free(buf);
 *   return SLOWLIBS_IO_OK;
 *
 *
 * Also see:
 * - slowlibs_transfer_chunk_size()
 * - slowlibs_transfer()
 */
slowlibs_io_status slowlibs_transfer_noyield(slowlibs_writer out,
                                             slowlibs_reader in,
                                             uint8_t* buf,
                                             size_t buflen,
                                             size_t* int_unwritten);

/**
 * Returns the optimal chunk size for transfers between these two.
 * Always returns at least one, and at most a reasonable allocatable amount.
 */
size_t slowlibs_transfer_chunk_size(slowlibs_writer out, slowlibs_reader in);

#endif
