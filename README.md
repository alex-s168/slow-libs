# slow-libs minimal libraries
0BSD licennce (see ./LICENSE)

Most libraries are not actually slow, but that is just the naming scheme.

Each library is documentated at the top of each header file

## Features of all libraries
- C89 (or later) compatible
- intuitive to use
- don't pollude namespace
- configurable with pre-processor macros


## Libraries
- `./include/slowlibs/chacha20.h`
- `./include/slowlibs/slowarr.h`: C templated dynamic array
- `./include/slowlibs/slowgraph.h`: WIP graph library (this is the only library that is actually slow)
- `./include/slowlibs/csv.h`
- `./include/slowlibs/systemrand.h`

there are more files than that, but those are heavility wip


## CLI Tools
CLI utils built with slow-libs.

- `./slowcrypt/slowcrypt.c`: cryptography cli
- `./slowgraph/sgraph-adj.c`: dgtxt graphs <-> adjacency matrix


## Cryptography Security Considerations
All cryptography libraries list possible side channel attacks in their documentation

| Library      | Zeroing memory | Timing attacks | Notes |
| ------------ | -------------- | -------------- | ----- |
| `chacha20.h` | manual [^1]    | impossible     | [^2]  |

[^1]: the compiler might optimize the zeroing away
[^2]: does not prevent against length-extension, by algorithm design. See Cryptography 101.


## Cryptography 101
- TODO: explain asymetric
- TODO: compare symetrics
- TODO: compare asymetrics
- TODO: compare hashes
- TODO: XOF
- TODO: rng
- TODO: length extension
- TODO: password hashing


## Contribution guide
- Please use `clang-format` (and the provided `.clang-format` style)
  If you do not want to use `clang-format`, try to match the code style of other existing code,
  and a maintainer will format your code on merge.
- Do not break portability of an existing "finished" library
- Make new libraries as portable as possible (in a reasonable amount of time)
