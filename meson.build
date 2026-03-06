project('slowlibs', 'c', 'cpp',
  version: '1.0.0-pre.1')

install_headers(
  './include/slowlibs/chacha20.h',
  './include/slowlibs/csv.h',
  './include/slowlibs/poly1305.h',
  './include/slowlibs/fixed_bigint.h',
  './include/slowlibs/util.h',
  './include/slowlibs/slowarr.h',
  './include/slowlibs/slowgraph.h',
  './include/slowlibs/systemrand.h',
  subdir: 'slowlibs')

slowlibs_headeronly_dep = declare_dependency(
  include_directories: 'include')
meson.override_dependency('slowlibs-headeronly', slowlibs_headeronly_dep)

libslowlibs = static_library('slowlibs',
  'src/io.c',
  'src/include_impl.c',
  install: true,
  dependencies: [slowlibs_headeronly_dep])

slowlibs_dep = declare_dependency(
  include_directories: 'include',
  link_with: libslowlibs)
meson.override_dependency('slowlibs', slowlibs_dep)


executable('slowcrypt',
  './src/slowcrypt/cli-slowcrypt.c',
  dependencies: [slowlibs_headeronly_dep],
  install: true)

executable('sgraph-adj',
  './src/slowgraph/cli-sgraph-adj.c',
  dependencies: [slowlibs_headeronly_dep],
  install: true)


if not meson.is_subproject()
  test('chacha20-block_test_vector', executable('chacha20-block_test_vector',
    './tests/chacha20/block_test_vector.c',
    dependencies: [slowlibs_headeronly_dep]))

  test('chacha20-keygen_test_vector', executable('chacha20-keygen_test_vector',
    './tests/chacha20/keygen_test_vector.c',
    dependencies: [slowlibs_headeronly_dep]))
    
  test('poly1305-test_vector_bitint', executable('poly1305-test_vector_bitint',
    './tests/poly1305/test_vector_bitint.c',
    dependencies: [slowlibs_headeronly_dep]))

  test('poly1305-test_vector_fbig', executable('poly1305-test_vector_fbig',
    './tests/poly1305/test_vector_fbig.c',
    dependencies: [slowlibs_headeronly_dep]))

  test('slowarr-nostd.1', executable('slowarr-nostd.1',
    './tests/slowarr/nostd1.c',
    dependencies: [slowlibs_headeronly_dep]))

  test('slowarr-std.1', executable('slowarr-std.1',
    './tests/slowarr/std1.c',
    dependencies: [slowlibs_headeronly_dep]))

  test('slowarr-std.1-cxx', executable('slowarr-std.1-cxx',
    './tests/slowarr/std1.cxx',
    dependencies: [slowlibs_headeronly_dep]))

  test('io-buffered-reader', executable('io-buffered-reader',
    './tests/io/buffered_reader.c',
    dependencies: [slowlibs_dep]))
endif
