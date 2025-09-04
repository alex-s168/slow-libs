#define SLOWGRAPH_IMPL
#include "../slowgraph.h"

static void write_csv_field(char const *s) {
  char const *y;
  int quote = 0;
  for (y = s; *y; y++) {
    if (*y == '\n' || *y == '"') {
      quote = 1;
      break;
    }
  }

  if (quote)
    putchar('"');

  for (; *s; s++) {
    if (*s == '"')
      putchar('"');
    putchar(*s);
  }

  if (quote)
    putchar('"');
}

static void dump_adj_matrix_row(SlowGraph *g, SlowGraphNode *row) {
  SlowGraphNode *col;
  write_csv_field(row->name);
  printf(",");
  for (col = g->first; col; col = col->next) {
    if (SlowGraphNode_findConnection(row, col)) {
      printf("1,");
    } else {
      printf("0,");
    }
  }
  printf("\n");
}

static void dump_adj_matrix(SlowGraph *g) {
  SlowGraphNode *n;
  printf(",");
  for (n = g->first; n; n = n->next) {
    if (n != g->first)
      printf(",");
    write_csv_field(n->name);
  }
  printf("\n");
  for (n = g->first; n; n = n->next) {
    dump_adj_matrix_row(g, n);
  }
}

int main(int argc, char **argv) {
  SlowGraph graph = {0};

  if (SlowGraph_readDGTXT(&graph, stdin))
    return 1;

  dump_adj_matrix(&graph);
  return 0;
}
