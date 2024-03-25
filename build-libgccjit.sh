# This should be merged to a bazel script
# fuck bazel anyways
# also please run git submodule update before this
set -e
mkdir -p build
mkdir -p install
PREFIX=$(pwd)/install
cd build
../gcc-src/configure \
  --enable-host-shared \
  --enable-languages=jit \
  --disable-bootstrap \
  --enable-checking=release \
  --prefix=$PREFIX
nice make -j4
make install
cd ..
echo "libgccjit should be at build/gcc/libgccjit.so* and install/lib/libgccjit.so*"
file build/gcc/libgccjit.so*
file install/lib/libgccjit.so*
read -p "Test? (need runtest/dejagnu) (y/n)?" choice
case "$choice" in 
  y|Y ) echo "ogey";;
  n|N ) exit 0;;
  * ) exit 0;;
esac

cd build/gcc
make check-jit RUNTESTFLAGS="-v -v -v"
cat jit/build/gcc/testsuite/jit/jit.sum
cd ../..
exit 0
