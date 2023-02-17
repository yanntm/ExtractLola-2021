#! /bin/sh


mkdir -p usr/
mkdir -p usr/local/
mkdir -p usr/local/bin/
mkdir -p usr/local/share/man/man1/

export INSTALL_DIR=$(pwd)/usr/local/

export PATH=$PWD/usr/local/bin/:$PATH

# kimwitu dependency
tar xvzf kimwitu++-2.3.13.tar.gz
cd kimwitu++-2.3.13/
./configure --prefix=$INSTALL_DIR
make && make install
cd ..



cd lola2
autoreconf -vfi
./configure --prefix=$INSTALL_DIR
make
make install
cd ..

 
cd sara/
./configure --prefix=$INSTALL_DIR
make && make install
cd ..

mkdir website
cp usr/local/bin/lola website/
cp usr/local/bin/sara website/
  

