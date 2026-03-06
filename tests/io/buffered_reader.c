#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include "slowlibs/io.h"

typedef struct
{
  uint8_t counter;
  size_t chunk_size, last_chunk_size;
  size_t num_chunks_rem;
} chunked_only_reader;

static slowlibs_io_status chunked_only_reader__read(size_t* len_out,
                                                    void* ctx,
                                                    uint8_t* buf,
                                                    size_t read_max)
{
  chunked_only_reader* r = ctx;
  
  if (!r->num_chunks_rem) { // If no chunks remaining, return END
    *len_out = 0;
    return SLOWLIBS_IO_READ_END;
  }
  
  r->num_chunks_rem--; // Decrement chunk counter

  size_t current_chunk_len;
  if (!r->num_chunks_rem) { // This is the LAST chunk
    current_chunk_len = r->last_chunk_size;
  } else { // Not the last chunk
    current_chunk_len = r->chunk_size;
  }

  // Determine how many bytes to actually read, limited by requested read_max and current_chunk_len
  size_t bytes_to_read = current_chunk_len;
  if (bytes_to_read > read_max) { // If chunk is larger than requested buffer
      bytes_to_read = read_max;
      // This is a partial read of the current chunk, so status should be ONLY_CHUNKED
  }

  // Fill the buffer with data
  for (size_t i = 0; i < bytes_to_read; i++)
    buf[i] = r->counter++;
  
  *len_out = bytes_to_read; // Set the number of bytes read

  if (!r->num_chunks_rem && bytes_to_read == current_chunk_len) { // If this was the last chunk and we read it all
    return SLOWLIBS_IO_READ_END;
  } else {
    // If we read less than a full chunk because read_max was smaller
    if (bytes_to_read < current_chunk_len) {
        return SLOWLIBS_IO_ONLY_CHUNKED;
    } else {
        return SLOWLIBS_IO_OK; // We read a full chunk
    }
  }
}

slowlibs_reader new_chunked_only_reader(size_t num_chunks,
                                        size_t chunk_size,
                                        size_t last_chunk_size)
{
  chunked_only_reader* state = malloc(sizeof(chunked_only_reader));
  assert(state);
  state->counter = 0;
  state->chunk_size = chunk_size;
  state->last_chunk_size = last_chunk_size;
  state->num_chunks_rem = num_chunks;

  return (slowlibs_reader){
      .ctx = state,
      .read = chunked_only_reader__read,
      .recommended_chunk_size = chunk_size,
      .close = free,
  };
}

void test(size_t max_transfer_chunk_sz)
{
  slowlibs_reader buffered; // FIRST declaration, KEEP this one
  if (slowlibs_buffered_reader_owned(&buffered,
                                     new_chunked_only_reader(3, 4, 1), 4))
    assert(0 && "malloc fail");

  // Buffer size needs to be sufficient for expected reads.
  // Current reader produces 9 bytes. Let's make buffer slightly larger.
  uint8_t data[9 + 10] = {0}; // Buffer size 19 bytes
  size_t wr = 0; // Total bytes read and accumulated

  // Loop until we have accumulated expected number of bytes (9 bytes) OR reached end of stream.
  while (wr < 9) {
    size_t bytes_to_read_this_round = sizeof(data) - wr;
    if (bytes_to_read_this_round > max_transfer_chunk_sz)
      bytes_to_read_this_round = max_transfer_chunk_sz;

    size_t num_read_in_call = 0;
    slowlibs_io_status status;

    // Call slowlibs_read once. Process status and continue if OK or ONLY_CHUNKED.
    status = slowlibs_read(&num_read_in_call, buffered, data + wr, bytes_to_read_this_round);

    if (status == SLOWLIBS_IO_OK || status == SLOWLIBS_IO_ONLY_CHUNKED) {
        wr += num_read_in_call;
        // If we've read enough bytes, we can break the outer loop.
        if (wr >= 9) break;
        // If ONLY_CHUNKED was returned, it means a partial read occurred due to buffer size.
        // The outer loop will continue to attempt reading more data if wr < 9.
    } else if (status == SLOWLIBS_IO_READ_END) {
        wr += num_read_in_call;
        // Reached end of stream. Break the outer loop.
        break;
    } else {
        // Any other status is an unexpected error.
        assert(0 && "Unexpected status from slowlibs_read");
    }
    // Ensure some progress was made if not EOF.
    if (wr < 9 && status != SLOWLIBS_IO_READ_END) {
         assert(num_read_in_call > 0);
    }
  }

  // Verify that exactly 9 bytes were read and accumulated.
  assert(wr == 9);

  // Verify the content of the first 9 bytes.
  for (size_t i = 0; i < 9; i++) // Verifying up to 9 bytes
    assert(i == (size_t)data[i]);

  // Loop for exhausting the reader
  for (size_t i = 0; i < 32; i++) {
    size_t num = 6969;
    slowlibs_io_status status;
    while ((status = slowlibs_read(&num, buffered, data, sizeof(data))) >
           SLOWLIBS_IO_READ_END)
      ;
    assert(status == SLOWLIBS_IO_READ_END);
    assert(num == 0);
  }

  slowlibs_close(buffered);
}

int main()
{
  test(SIZE_MAX);  /* more than all at once */
  test(3 * 4 + 1); /* exactly all at once */
  test(5);         /* more than one chunk at once */
  test(4);         /* excatly one chunk at once */
  test(2);         /* less than one chunk at once */
}
