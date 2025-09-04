/* define SLOWGRAPH_IMPL */

#ifndef SLOWGRAPH_COMMON_H
#define SLOWGRAPH_COMMON_H

#ifndef SLOWGRAPH_FUNC
#define SLOWGRAPH_FUNC /**/
#endif

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SLOWGRAPH_LENOF(x) (sizeof((x)) / sizeof(*(x)))

typedef struct SlowGraph SlowGraph;
typedef struct SlowGraphNode SlowGraphNode;
typedef struct SlowGraphEdge SlowGraphEdge;
typedef struct SlowGraphAttr SlowGraphAttr;

struct SlowGraphAttr {
  SlowGraphAttr *next;
  unsigned hash;
  char *key, *val;
};

struct SlowGraphNode {
  SlowGraphNode *next;

  unsigned name_hash;
  char *name;
  SlowGraphAttr *attr;
  SlowGraphEdge *children;

  unsigned gc_used : 1;
};

struct SlowGraphEdge {
  SlowGraphEdge *next;
  SlowGraphAttr *attr;

  SlowGraphNode *node;
};

struct SlowGraph {
  SlowGraphNode *first;
};

SLOWGRAPH_FUNC void SlowGraphAttr_free(SlowGraphAttr *attrs);

SLOWGRAPH_FUNC unsigned slowgraph_hash(char const *buf);

SLOWGRAPH_FUNC unsigned slowgraph_hashn(void const *bufp, int len);

SLOWGRAPH_FUNC char *
slowgraph_next_edge(FILE *inp, char *buf, int bufn,
                    void (*opt_attr_clbk)(char *node, char *key, char *val),
                    void (*opt_last_edge_attr_clbk)(char *key, char *val));

SLOWGRAPH_FUNC SlowGraphAttr *SlowGraphAttr_find(SlowGraphAttr *a,
                                                 unsigned hash);

/*
 Step 1: call SlowGraph_markAllUnused(graph)

 Step 2: manually mark input nodes as used

 Step 3: call SlowGraph_gcUnused(graph)   // only deletes nodes not used by
 manually marked used nodes
 */
SLOWGRAPH_FUNC void SlowGraph_markAllUnused(SlowGraph *graph);
SLOWGRAPH_FUNC void SlowGraph_gcUnused(SlowGraph *graph);

SLOWGRAPH_FUNC SlowGraphNode *SlowGraph_find(SlowGraph *g, unsigned name_hash);

SLOWGRAPH_FUNC SlowGraphEdge *SlowGraphNode_findConnection(SlowGraphNode *from,
                                                           SlowGraphNode *to);

/* if edge from->to already exists, return it instead */
SLOWGRAPH_FUNC SlowGraphEdge *SlowGraphNode_connect(SlowGraphNode *from,
                                                    SlowGraphNode *to);

SLOWGRAPH_FUNC SlowGraphNode *SlowGraph_getOrCreate(SlowGraph *g,
                                                    char const *name);

SLOWGRAPH_FUNC void slowgraph_setAttr(SlowGraphAttr **list, char const *key,
                                      char const *val);

/* returns 1 on failure */
SLOWGRAPH_FUNC int SlowGraph_readDGTXT(SlowGraph *g, FILE *f);

#ifdef SLOWGRAPH_IMPL
SLOWGRAPH_FUNC void SlowGraphAttr_free(SlowGraphAttr *attrs) {
  SlowGraphAttr *to_free;
  for (; attrs;) {
    to_free = attrs;
    attrs = attrs->next;
    free(to_free->key);
    free(to_free->val);
    free(to_free);
  }
}

SLOWGRAPH_FUNC unsigned slowgraph_hash(char const *buf) {
  unsigned res = 5381;
  for (; *buf; buf++) {
    res = (res << 5) + res + *buf;
  }
  return res;
}

SLOWGRAPH_FUNC unsigned slowgraph_hashn(void const *bufp, int len) {
  char const *buf = (char const *)bufp;
  unsigned res = 5381;
  for (; buf != ((char const *)bufp) + len; buf++) {
    res = (res << 5) + res + *buf;
  }
  return res;
}

SLOWGRAPH_FUNC char *
slowgraph_next_edge(FILE *inp, char *buf, int bufn,
                    void (*opt_attr_clbk)(char *node, char *key, char *val),
                    void (*opt_last_edge_attr_clbk)(char *key, char *val)) {
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
        n = strtok(buf + 1, " \r\n");
        k = strtok(0, " \r\n");
        v = strtok(0, " \r\n");
        opt_attr_clbk(n, k, v);
      } else if (c == ':' && opt_last_edge_attr_clbk &&
                 buf == fgets(buf, bufn, inp)) {
        k = strtok(buf + 1, " \r\n");
        v = strtok(0, " \r\n");
        opt_last_edge_attr_clbk(k, v);
      } else {
        for (; c != EOF && c != '\n'; c = fgetc(inp))
          ;
        c = fgetc(inp);
      }
      continue;
    }

    for (k = buf; c != EOF && !isspace(c); k++) {
      *k = c;
      c = fgetc(inp);
    }

    if (k == buf)
      continue;

    *k++ = 0;

    for (; isspace(c); c = fgetc(inp))
      ;

    for (v = k; c != EOF && !isspace(c); v++) {
      *v = c;
      c = fgetc(inp);
    }
    *v = 0;

    if (v == k)
      continue;

    retv = k;
    break;
  }

  ungetc(c, inp);
  return retv;
}

SLOWGRAPH_FUNC SlowGraphAttr *SlowGraphAttr_find(SlowGraphAttr *a,
                                                 unsigned hash) {
  for (; a; a = a->next)
    if (a->hash == hash)
      return a;
  return 0;
}

SLOWGRAPH_FUNC void SlowGraph_markAllUnused(SlowGraph *graph) {
  SlowGraphNode *n;
  for (n = graph->first; n; n = n->next)
    n->gc_used = 0;
}

SLOWGRAPH_FUNC void SlowGraph_gcUnused(SlowGraph *graph) {
  int changed = 0;
  SlowGraphNode *n, *o, **prev;
  SlowGraphEdge *e, *oe;

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
  for (n = graph->first; n;) {
    if (n->gc_used) {
      prev = &n->next;
      n = n->next;
    } else {
      o = n;
      n = n->next;
      *prev = n;

      for (e = o->children; e;) {
        oe = e;
        e = e->next;
        SlowGraphAttr_free(oe->attr);
        free(oe);
      }

      SlowGraphAttr_free(o->attr);
      free(o->name);
      free(o);
    }
  }
}

SLOWGRAPH_FUNC SlowGraphNode *SlowGraph_find(SlowGraph *g, unsigned name_hash) {
  SlowGraphNode *n;
  for (n = g->first; n; n = n->next)
    if (n->name_hash == name_hash)
      return n;
  return 0;
}

SLOWGRAPH_FUNC SlowGraphEdge *SlowGraphNode_findConnection(SlowGraphNode *from,
                                                           SlowGraphNode *to) {
  SlowGraphEdge *e;
  for (e = from->children; e; e = e->next)
    if (e->node == to)
      return e;
  return 0;
}

SLOWGRAPH_FUNC SlowGraphEdge *SlowGraphNode_connect(SlowGraphNode *from,
                                                    SlowGraphNode *to) {
  SlowGraphEdge *e = SlowGraphNode_findConnection(from, to);
  if (e)
    return e;
  e = (SlowGraphEdge *)calloc(1, sizeof(SlowGraphEdge));
  if (!e)
    return 0;
  e->node = to;
  e->next = from->children;
  from->children = e;
  return e;
}

SLOWGRAPH_FUNC SlowGraphNode *SlowGraph_getOrCreate(SlowGraph *g,
                                                    char const *name) {
  unsigned hash, strln;
  SlowGraphNode *n;

  strln = strlen(name);
  hash = slowgraph_hashn(name, strln);
  if ((n = SlowGraph_find(g, hash)))
    return n;
  n = (SlowGraphNode *)calloc(1, sizeof(SlowGraphNode));
  if (!n)
    return 0;
  n->name = (char *)calloc(strln + 1, sizeof(char));
  if (!n->name) {
    free(n);
    return 0;
  }
  strcpy(n->name, name);
  n->name_hash = hash;
  n->next = g->first;
  g->first = n;
  return n;
}

SLOWGRAPH_FUNC void slowgraph_setAttr(SlowGraphAttr **list, char const *key,
                                      char const *val) {
  /* TODO: malloc fail handking */
  unsigned keylen = strlen(key);
  unsigned vallen = strlen(val);
  unsigned hash = slowgraph_hashn(key, keylen);
  SlowGraphAttr *a = SlowGraphAttr_find(*list, hash);
  if (a) {
    free(a->val);
  } else {
    a = (SlowGraphAttr *)calloc(1, sizeof(SlowGraphAttr));
    a->next = *list;
    *list = a;

    a->hash = hash;
    a->key = (char *)calloc(keylen + 1, sizeof(char));
    strcpy(a->key, key);
  }

  a->val = (char *)calloc(vallen + 1, sizeof(char));
  strcpy(a->val, val);
}

static SlowGraph *SlowGraph_read__graph;
static SlowGraphEdge *SlowGraph_read__lastEdge;

static void SlowGraph_read__nodeAttr(char *node, char *key, char *val) {
  SlowGraphNode *n = SlowGraph_getOrCreate(SlowGraph_read__graph, node);
  if (!n)
    return;
  slowgraph_setAttr(&n->attr, key, val);
}

static void SlowGraph_read__edgeAttr(char *key, char *val) {
  if (!SlowGraph_read__lastEdge)
    return;
  slowgraph_setAttr(&SlowGraph_read__lastEdge->attr, key, val);
}

/* returns 1 on failure */
SLOWGRAPH_FUNC int SlowGraph_readDGTXT(SlowGraph *g, FILE *f) {
  static char buf[512];
  char *dest;

  SlowGraph_read__graph = g;
  SlowGraph_read__lastEdge = 0;

  while ((dest = slowgraph_next_edge(f, buf, SLOWGRAPH_LENOF(buf),
                                     SlowGraph_read__nodeAttr,
                                     SlowGraph_read__edgeAttr))) {

    SlowGraph_read__lastEdge = SlowGraphNode_connect(
        SlowGraph_getOrCreate(g, buf), SlowGraph_getOrCreate(g, dest));
    if (!SlowGraph_read__lastEdge)
      return 1;
  }

  SlowGraph_read__graph = 0;
  SlowGraph_read__lastEdge = 0;

  return 0;
}

#endif

#endif
