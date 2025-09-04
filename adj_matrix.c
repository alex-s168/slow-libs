#include "common.h"

int main(int argc, char **argv) {
  char buf[512];
  char *dest;

  while (( dest = slowgraph_next_edge(stdin, buf, slowgraph_lenof(buf), 0) )) {
    printf("%s -> %s\n", buf, dest);
  }

  return 0;
}
