# slow-libs single-header libraries
MIT / Public Domain (see ./LICENSE)

Most libraries are not actually slow, but that is just the naming scheme.

## Features of all libraries
- C89 (or later) compatible
- intuitive to use
- don't pollude namespace
- configurable with pre-processor macros

## Libraries
- `./chacha20.h`
- `./slowarr.h`: C templated dynamic array
- `./slowgraph.h`: WIP graph library (this is the only library that is actually slow)

## CLI Tools
CLI utils built with slow-libs.

- `./slowgraph/sgraph-adj.c`: dgtxt graphs <-> adjacency matrix

