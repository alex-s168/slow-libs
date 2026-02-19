/*
 * Example 1: Read CSV cell-by-cell into fixed size buffer
 *   slowcsv_state csv = slowcsv_init(stdin);
 *   while (slowcsv_next_row(&csv)) {
 *     while (slowcsv_next_cell(&csv)) {
 *       char data[100];
 *       slowcsv_next_chunk(&csv, data, 100);
 *       if (slowcsv_have_chunk(&csv)) {
 *         perror("too much data in cell");
 *       }
 *     }
 *   }
 *
 *
 * Example 2: Read CSV cell-by-cell into dynamic buffer
 *   slowcsv_state csv = slowcsv_init(stdin);
 *   while (slowcsv_next_row(&csv)) {
 *     while (slowcsv_next_cell(&csv)) {
 *       char* data = 0;
 *       unsigned datalen = 0;
 *       while ( slowcsv_have_chunk(&csv) ) {
 *         data = realloc(data, datalen + 64);
 *         slowcsv_next_chunk(&data[datalen], 64);
 *       }
 *       // data will never be 0. all cells have at least one chunk, containing the null terminator
 *     }
 *   }
 *
 *
 * Example 3: Read CSV cell-by-cell into fixed size buffer, skipping the first column
 *   slowcsv_state csv = slowcsv_init(stdin);
 *   while (slowcsv_next_row(&csv)) {
 *     assert( slowcsv_next_cell(&csv) );
 *     slowcsv_skip_cell(&csv);
 *
 *     while (slowcsv_next_cell(&csv)) {
 *       ...
 *       slowcsv_next_chunk(&csv, data, 100);
 *       ...
 *     }
 *   }
 *
 * 
 * Example 4: Canonicalize CSV
 *   slowcsv_state reader = slowcsv_init(stdin);
 *   slowcsv_state writer = slowcsv_init(stdout);
 *
 *   while (slowcsv_next_row(&reader)) {
 *     while (slowcsv_next_cell(&reader)) {
 *       char* data = 0;
 *       unsigned datalen = 0;
 *       while ( slowcsv_have_chunk(&reader) ) {
 *         data = realloc(data, datalen + 64);
 *         slowcsv_next_chunk(&data[datalen], 64);
 *       }
 *       slowcsv_write_cell(&writer, data);
 *       free(data);
 *     }
 *     slowcsv_write_rowend(&writer);
 *   }
 *
 *
 */

#ifndef SLOWCSV_H
#define SLOWCSV_H

#ifndef SLOWCSV_FUNC
#define SLOWCSV_FUNC /**/
#endif

#include <stdio.h>

typedef struct
{
  FILE* fp;
  /* configurable; default: ',' */
  char separator;
  /* configurable; default: '"' */
  char quote;
  /* configurable; default: false */
  int quote_when_spaces_in_middle;

  /* private */
  int _reader_in_quoted;
  /* private */
  int _reader_null_avail;
  /* private */
  int _writer_first_in_row;
} slowcsv_state;

SLOWCSV_FUNC slowcsv_state slowcsv_init(FILE* fp);

/* ==== reader ==== */

/* returns 1 if a row was found, otherwise 0 */
SLOWCSV_FUNC int slowcsv_next_row(slowcsv_state* state);

/* returns 1 if a cell was found in this row, otherwise 0 */
SLOWCSV_FUNC int slowcsv_next_cell(slowcsv_state* state);

/* skip the current cell */
SLOWCSV_FUNC void slowcsv_skip_cell(slowcsv_state* state);

SLOWCSV_FUNC int slowcsv_have_chunk(slowcsv_state* state);

/* an empty cell will always have one available chunk, containing the null terminator */
SLOWCSV_FUNC void slowcsv_next_chunk(slowcsv_state* state,
                                     char* buf,
                                     unsigned long buflen);

/* the buffer will always be 0 terminated, even if cell longer than buffer, as long as buflen > 0.
 *
 * returns 1 if the cell didn't fit, in which case the remaining data from this cell can't be read anymore, and you have to call next_cell().
 */
SLOWCSV_FUNC int slowcsv_whole_cell(slowcsv_state* state,
                                    char* buf,
                                    unsigned long buflen);

/* ==== writer ==== */

SLOWCSV_FUNC void slowcsv_write_cell(slowcsv_state* state, char const* text);
SLOWCSV_FUNC void slowcsv_write_rowend(slowcsv_state* state);

#ifdef SLOWCSV_IMPL

#include <ctype.h>

SLOWCSV_FUNC slowcsv_state slowcsv_init(FILE* fp)
{
  slowcsv_state state = {0};
  state.fp = fp;
  state._writer_first_in_row = 1;
  state.separator = ',';
  state.quote = '"';
  return state;
}

static int slowcsv__read1(slowcsv_state* state)
{
  int c;

  for (;;) {
    c = fgetc(state->fp);
    if (c == state->quote) {
      if (state->_reader_in_quoted) {
        c = fgetc(state->fp);
        if (c == state->quote) {
          state->_reader_in_quoted = 0;
          return c;
        }
        ungetc(c, state->fp);
        return 0;
      } else {
        state->_reader_in_quoted = 1;
        continue;
      }
    }
    if (!state->_reader_in_quoted && c == state->separator) {
      ungetc(c, state->fp);
      return 0;
    }
    return c;
  }
}

SLOWCSV_FUNC int slowcsv_have_chunk(slowcsv_state* state)
{
  if (state->_reader_null_avail)
    return 1;
  int c = fgetc(state->fp);
  ungetc(c, state->fp);
  return c != EOF &&
         (state->_reader_in_quoted || (c != ',' && c != '\r' & c != '\n'));
}

/* an empty cell will always have one available chunk, containing the null terminator */
SLOWCSV_FUNC void slowcsv_next_chunk(slowcsv_state* state,
                                     char* buf,
                                     unsigned long buflen)
{
  int c;
  if (!buflen)
    return;

  if (state->_reader_null_avail) {
    *buf = 0;
    ++buf;
    --buflen;
    state->_reader_null_avail = 0;
    return;
  }

  while (buflen) {
    c = slowcsv__read1(state);
    if (c == EOF || c == 0) {
      state->_reader_null_avail = 1;
      break;
    }
    *buf++ = c;
    --buflen;
  }
}

SLOWCSV_FUNC int slowcsv_whole_cell(slowcsv_state* state,
                                    char* buf,
                                    unsigned long buflen)
{
  int c;
  if (!buflen)
    return 1;

  state->_reader_null_avail = 0;

  do {
    if (!buflen)
      return 1;
    c = slowcsv__read1(state);
    *buf++ = c;
    --buflen;
  } while (c != EOF && c != 0);

  buf[-1] = 0;
  return 0;
}

SLOWCSV_FUNC void slowcsv_skip_cell(slowcsv_state* state)
{
  int c;
  do {
    c = slowcsv__read1(state);
  } while (c != EOF && c != 0);
}

SLOWCSV_FUNC int slowcsv_next_cell(slowcsv_state* state)
{
  int c;

  if (state->_reader_in_quoted) {
    slowcsv_skip_cell(state);
  }

  do {
    c = fgetc(state->fp);
    if (c == EOF)
      return 0;
    if (c == '\n')
      return 0;
  } while (isspace(c));
  ungetc(c, state->fp);
  return 1;
}

SLOWCSV_FUNC int slowcsv_next_row(slowcsv_state* state)
{
  int c;

  while (slowcsv_next_cell(state))
    ;

  do {
    c = fgetc(state->fp);
    if (c == EOF)
      return 0;
  } while (isspace(c));
  ungetc(c, state->fp);
  return 1;
}

SLOWCSV_FUNC void slowcsv_write_cell(slowcsv_state* state, char const* text)
{
  char const* p;
  int quote = 0;
  int had_nonspace = 0;

  if (!state->_writer_first_in_row)
    fputc(state->separator, state->fp);

  for (p = text; *p; p++) {
    if (isspace(*p)) {
      if (!had_nonspace || state->quote_when_spaces_in_middle) {
        quote = 1;
        break;
      }
    } else {
      had_nonspace = 1;
    }
    if (*p == '\n' || *p == '\r' || *p == '\t' || *p == state->separator ||
        *p == state->quote) {
      quote = 1;
      break;
    }
  }
  if (quote)
    fputc(state->quote, state->fp);
  for (p = text; *p; p++) {
    if (*p == state->quote) {
      fputc(state->quote, state->fp);
      fputc(state->quote, state->fp);
    } else {
      fputc(*p, state->fp);
    }
  }
  if (quote)
    fputc(state->quote, state->fp);
  state->_writer_first_in_row = 0;
}

SLOWCSV_FUNC void slowcsv_write_rowend(slowcsv_state* state)
{
  fputc('\n', state->fp);
  state->_writer_first_in_row = 1;
}

#endif

#endif
