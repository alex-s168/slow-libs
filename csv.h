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
  /* configurable */
  char separator;
  /* configurable */
  int quote_when_spaces;

  /* private */
  int _reader_in_quoted;
  /* private */
  int _writer_first_in_row;
} slowcsv_state;

SLOWCSV_FUNC slowcsv_state slowcsv_init(FILE* fp);

/* ==== reader ==== */

/* returns 1 if a row was found, otherwise 0 */
SLOWCSV_FUNC int slowcsv_next_row(slowcsv_state* state);

/* returns 1 if a cell was found, otherwise 0 */
SLOWCSV_FUNC int slowcsv_next_cell(slowcsv_state* state);

/* skip the current cell */
SLOWCSV_FUNC void slowcsv_skip_cell(slowcsv_state* state);

SLOWCSV_FUNC int slowcsv_have_chunk(slowcsv_state* state);

/* an empty cell will always have one available chunk, containing hte null terminator */
SLOWCSV_FUNC void slowcsv_next_chunk(slowcsv_state* state,
                                     char* buf,
                                     unsigned long buflen);

/* ==== writer ==== */

SLOWCSV_FUNC void slowcsv_write_cell(slowcsv_state* state, char const* text);
SLOWCSV_FUNC void slowcsv_write_rowend(slowcsv_state* state);

// TODO
// #ifdef SLOWCSV_IMPL

#include <ctype.h>

SLOWCSV_FUNC int slowcsv_next_row(slowcsv_state* state)
{
  if (feof(state->fp))
    return 0;
  return 1;
}

// TODO: next cell, and cell read fns

SLOWCSV_FUNC slowcsv_state slowcsv_init(FILE* fp)
{
  slowcsv_state state = {0};
  state.fp = fp;
  state._writer_first_in_row = 1;
  state.separator = ',';
  return state;
}

SLOWCSV_FUNC void slowcsv_write_cell(slowcsv_state* state, char const* text)
{
  char const* p;
  int quote = 0;

  if (!state->_writer_first_in_row)
    fputc(state->separator, state->fp);

  for (p = text; *p; p++) {
    if (state->quote_when_spaces && isspace(*p)) {
      quote = 1;
      break;
    }
    if (*p == '\n' || *p == '\r' || *p == '\t' || *p == state->separator ||
        *p == '"') {
      quote = 1;
      break;
    }
  }
  if (quote)
    fputc('"', state->fp);
  for (p = text; *p; p++) {
    if (*p == '"') {
      fputc('"', state->fp);
      fputc('"', state->fp);
    } else {
      fputc(*p, state->fp);
    }
  }
  if (quote)
    fputc('"', state->fp);
  state->_writer_first_in_row = 0;
}

SLOWCSV_FUNC void slowcsv_write_rowend(slowcsv_state* state)
{
  fputc('\n', state->fp);
  state->_writer_first_in_row = 1;
}

// #endif

#endif
