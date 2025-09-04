set -e

run () {
  set -e
  echo "# $@"
  $@
}

echo "# ==== clang NATIVE ===="
for f in */std*.c; do
  for std in c89 c90 c99 c11 c17 c23 gnu89 gnu99 gnu11 gnu17 gnu23 c2y gnu2y; do
    run clang $f -Wall -Wextra -Wpedantic -Werror -Wno-extra-semi -std=$std --analyze -O0 -o /dev/null
  done
done

echo
echo "# ==== clang rv32 FREESTANDING ===="
for f in */nostd*.c; do
  for std in c89 c90 c99 c11 c17 c23 gnu89 gnu99 gnu11 gnu17 gnu23 c2y gnu2y; do
    run clang $f -Wall -Wextra -Wpedantic -Werror -Wno-extra-semi -std=$std --analyze -O0 -target riscv32-unknown-freestanding -nostdlib -o /dev/null
  done
done

echo
echo "# ==== clang wasm32 FREESTANDING ===="
for f in */nostd*.c; do
  for std in c89 c90 c99 c11 c17 c23 gnu89 gnu99 gnu11 gnu17 gnu23 c2y gnu2y; do
    run clang $f -Wall -Wextra -Wpedantic -Werror -Wno-extra-semi -std=$std --analyze -O0 -target wasm32-unknown-freestanding -nostdlib -o /dev/null
  done
done

echo
echo "# ==== clang++ NATIVE ===="
for f in */std*.cxx; do
  for std in c++03 c++98 gnu++98 c++11 gnu++11 c++14 c++17 gnu++17 c++20 gnu++20 c++23 gnu++23 c++2c gnu++2c; do
    run clang++ $f -Wall -Wextra -Wpedantic -Werror -Wno-extra-semi -std=$std --analyze -O0 -o /dev/null
  done
done

echo
echo "# ==== g++ NATIVE ===="
for f in */std*.cxx; do
  for std in c++03 c++98 gnu++98 c++11 gnu++11 c++14 c++17 gnu++17 c++20 gnu++20 c++23 gnu++23 c++2c gnu++2c; do
    run g++ $f -Wall -Wextra -Wpedantic -Werror -std=$std -fanalyzer -O0 -o /dev/null
  done
done

echo
echo "# ==== gcc NATIVE ===="
for f in */std*.c; do
  for std in c89 c90 c99 c11 c17 c23 gnu89 gnu99 gnu11 gnu17 gnu23; do
    run gcc $f -Wall -Wextra -Wpedantic -Werror -std=$std -fanalyzer -O0 -o /dev/null
  done
done

echo
echo "# ==== tcc NATIVE ===="
for f in */std*.c; do
  run tcc $f -Wall -Werror -o /dev/null
done
