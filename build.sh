#! /bin/sh


mkdir -p usr/
mkdir -p usr/local/
mkdir -p usr/local/bin/
mkdir -p usr/local/share/man/man1/

export INSTALL_DIR=$(pwd)/usr/local/

# build
cd lola/bin

# kimwitu dependency
cd kimwitu++-2.3.13/
./configure --prefix=$INSTALL_DIR
make && make install
cd ..
 
cd sara/
./configure --prefix=$INSTALL_DIR
make && make install
cd ..
 
cd lola/
# don't build doc
cp ../../../patches/Makefile.am .
# issues with new LTO enabled compilers
cp ../../../patches/generalized.c ./src/Formula/LTL/
autoreconf -vfi
./configure --prefix=$INSTALL_DIR
make && make install
cd ..



cd lola2
autoreconf -vfi
./configure 
# -L$DEPFOLDER/static-libs -L$DEPFOLDER/lib/ -L$DEPFOLDER/lib64/ -L$ROOTDIR/usr/local/lib/ -llzma 
make LDFLAGS="-all-static -static-libgcc -static-libstdc++"
