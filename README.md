# slow-libs minimal libraries
0BSD licennce (see ./LICENSE)

Most libraries are not actually slow, but that is just the naming scheme.

Each library is documentated at the top of each header file

## Features of all libraries
- C99 compatible
- intuitive to use
- don't pollude namespace
- configurable with pre-processor macros


## Libraries
- `./include/slowlibs/chacha20.h`
- `./include/slowlibs/poly1305.h`
- `./include/slowlibs/slowarr.h`: C templated dynamic array
- `./include/slowlibs/slowgraph.h`: WIP graph library (this is the only library that is actually slow)
- `./include/slowlibs/csv.h`
- `./include/slowlibs/systemrand.h`

Note there are lots of files lying around in this repository, most of which correspond to unfinished features.


## CLI Tools
CLI utils built with slow-libs.

- `./slowcrypt/cli-slowcrypt.c`: cryptography cli
- `./slowgraph/sgraph-adj.c`: dgtxt graphs <-> adjacency matrix


## Cryptography Security Considerations!
This is not the most serious crytography library. Do NOT use this in any sensitive applications!

Especially don't run this if an attacker might have access to process timing, memory, or exact power usage!

If you need more serious cryptography, see [Monocypher](https://monocypher.org/)

This library should however be fine for lots of applications, and we want to correct security issues. If you happen to find some, please report them immediately!

Additionally, we don't invest at all in performance optimizations, which shows significantly in
(currently unfinished) algorithms like Keccak, where we strictly followed the SHA-3 specification,
instead of applying optimization tricks.


### Attack channels
All cryptography libraries list possible side channel attacks in their documentation

| Library      | Zeroing memory | Timing attacks | Notes |
| ------------ | -------------- | -------------- | ----- |
| `chacha20.h` | manual [^1]    | impossible     | [^2]  |

[^1]: the compiler might optimize the zeroing away
[^2]: does not prevent against length-extension, by algorithm design. See Cryptography 101.

### Also see
our semi-custom cryptography:
  - [ChaCha-related](./doc/cacha20.md)

## Contribution guide
- Please use `clang-format` (and the provided `.clang-format` style)
  If you do not want to use `clang-format`, try to match the code style of other existing code,
  and a maintainer will format your code on merge.
- Do not break portability of an existing "finished" library
- Make new libraries as portable as possible (in a reasonable amount of time)
