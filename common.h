#ifndef SLOWGRAPH_COMMON_H
#define SLOWGRAPH_COMMON_H

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define slowgraph_lenof(x) (sizeof((x)) / sizeof(*(x)))

static unsigned
slowgraph_hash(char const *buf) {
  unsigned res = 5381;
  for (; *buf; buf++) {
    res = (res << 5) + res + *buf;
  }
  return res;
}

static unsigned
slowgraph_hashn(void const *bufp, int len) {
  char const* buf = (char const *) bufp;
  unsigned res = 5381;
  for (; buf != buf + len; buf ++) {
    res = (res << 5) + res + *buf;
  }
  return res;
}

static char *
slowgraph_next_edge(
    FILE* inp,
    char* buf, int bufn,
    void (*opt_attr_clbk)(char* node, char* key, char* val))
{
  int c;
  char *n, *k, *v, *retv = 0;

  c = fgetc(inp);

  for (;;) {
    if (c == EOF) {
      retv = 0;
      break;
    }

    for (; isspace(c); c = fgetc(inp))
      ;

    if (c == '#') {
      c = fgetc(inp);
      if (c == '#' && opt_attr_clbk && buf == fgets(buf, bufn, inp)) {
        n = strtok(buf+1, " \r\n");
        k = strtok(0, " \r\n");
        v = strtok(0, " \r\n");
        opt_attr_clbk(n, k, v);
      } else {
        for (; c != EOF && c != '\n'; c = fgetc(inp))
          ;
        c = fgetc(inp);
      }
      continue;
    }

    for (k = buf; c != EOF && !isspace(c); k ++) {
      *k = c;
      c = fgetc(inp);
    }

    *k++=0;

    for (; isspace(c); c = fgetc(inp))
      ;

    for (v = k; c != EOF && !isspace(c); v ++) {
      *v = c;
      c = fgetc(inp);
    }
    *v = 0;

    retv = k;
    break;
  }

  ungetc(c, inp);
  return retv;
}

typedef struct SlowGraph SlowGraph;
typedef struct SlowGraphNode SlowGraphNode;
typedef struct SlowGraphEdge SlowGraphEdge;
typedef struct SlowGraphAttr SlowGraphAttr;

struct SlowGraphAttr {
  SlowGraphAttr* next;
  unsigned hash;
  char *key, *val;
};

static char*
SlowGraphAttr_find(SlowGraphAttr* a, unsigned hash) {
  for (; a; a = a->next)
    if (a->hash == hash)
      return a->val;
  return 0;
}

struct SlowGraphNode {
  SlowGraphNode *next;

  unsigned name_hash;
  char *name;
  SlowGraphAttr* attr;
  SlowGraphEdge* children;

  unsigned gc_used : 1;
};

struct SlowGraphEdge {
  SlowGraphEdge *next;
  unsigned cost;
  unsigned aig_inv : 1;
  unsigned user_attr;

  SlowGraphNode *node;
};

struct SlowGraph {
  SlowGraphNode *first;
};

// Step 1: call SlowGraph_markAllUnused(graph)
// 
// Step 2: manually mark input nodes as used
//
// Step 3: call SlowGraph_gcUnused(graph)   // only deletes nodes not used by manually marked used nodes
static void
SlowGraph_markAllUnused(SlowGraph *graph) {
  SlowGraphNode *n;
  for (n = graph->first; n; n = n->next)
    n->gc_used = 0;
}

// Step 1: call SlowGraph_markAllUnused(graph)
// 
// Step 2: manually mark input nodes as used
//
// Step 3: call SlowGraph_gcUnused(graph)   // only deletes nodes not used by manually marked used nodes
static void
SlowGraph_gcUnused(SlowGraph *graph) {
  int changed = 0;
  SlowGraphNode *n, *o, **prev;
  SlowGraphEdge *e, *oe;
  SlowGraphAttr *a, *oa;

  do {
    for (n = graph->first; n; n = n->next) {
      if (n->gc_used) {
        for (e = n->children; e; e = e->next) {
          if (!e->node->gc_used) {
            e->node->gc_used = 1;
            changed = 1;
          }
        }
      }
    }
  } while (changed);

  prev = &graph->first;
  for (n = graph->first; n; ) {
    if (n->gc_used) {
      prev = &n->next;
      n = n->next;
    } else {
      o = n;
      n = n->next;
      *prev = n;

      for (e = o->children; e; ) {
        oe = e;
        e = e->next;
        free(oe);
      }

      for (a = o->attr; a; ) {
        oa = a;
        a = a->next;

        free(a->key);
        free(a->val);
        free(a);
      }

      free(o->name);
      free(o);
    }
  }
}

#endif
