#include <ctype.h>
#include <stdio.h>
#define SLOWGRAPH_IMPL
#include "slowlibs/slowgraph.h"
#define SLOWCSV_IMPL
#include "slowlibs/csv.h"

static void dump_adj_matrix_row(slowcsv_state* csv,
                                SlowGraph* g,
                                SlowGraphNode* row)
{
  SlowGraphNode* col;
  slowcsv_write_cell(csv, row->name);
  for (col = g->first; col; col = col->next) {
    slowcsv_write_cell(csv, SlowGraphNode_findConnection(row, col) ? "1" : "0");
  }
  slowcsv_write_rowend(csv);
}

static void dump_adj_matrix(FILE* out, SlowGraph* g)
{
  SlowGraphNode* n;
  slowcsv_state csv = slowcsv_init(out);

  slowcsv_write_cell(&csv, "");
  for (n = g->first; n; n = n->next) {
    slowcsv_write_cell(&csv, n->name);
  }
  slowcsv_write_rowend(&csv);

  for (n = g->first; n; n = n->next) {
    dump_adj_matrix_row(&csv, g, n);
  }
}

static char buf[512];

int main(int argc, char** argv)
{
  SlowGraph graph = {0};
  int ch, csv_inp;

  for (; (ch = fgetc(stdin)) && isspace(ch);)
    ;
  csv_inp = ch == ',';
  ungetc(ch, stdin);

  if (csv_inp) {
    slowcsv_state csv = slowcsv_init(stdin);
    if (!slowcsv_next_row(&csv))
      return 1;
    while (slowcsv_next_cell(&csv)) {
      if (!slowcsv_whole_cell(&csv, buf, sizeof(buf)))
        return 1;
      puts(buf);
    }
    while (slowcsv_next_row(&csv)) {
      // TODO
    }
  } else {
    if (SlowGraph_readDGTXT(&graph, buf, sizeof(buf), stdin))
      return 1;

    dump_adj_matrix(stdout, &graph);
  }

  return 0;
}
