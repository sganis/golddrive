#!/bin/sh
#
# build openssh

PREFIX=/home/sant/local
zlib=zlib-1.2.11
openssl=openssl-OpenSSL_1_1_1f
opnessh=openssh-portable-hpn-8_1_P1

export PATH=$PREFIX/bin:$PATH
export LD_LIBRARY_PATH=$PREFIX/lib:$LD_LIBRARY_PATH

[ -e $zlib ] && rm -rf $zlib
tar xvf $zlib.tar.gz
cd $zlib
./configure --prefix=$PREFIX --static
make -j8
make install
cd ..

[ -e $openssl ] && rm -rf $openssl
tar xvf $openssl.tar.gz
cd $openssl
./config --prefix=$PREFIX --openssldir=$PREFIX 
make -j8
make install_sw
make install_ssldirs
cd ..

[ -e $opnessh ] && rm -rf $opnessh
tar xvf $opnessh.tar.gz
cd $opnessh
autoconf
autoheader
./configure --prefix=$PREFIX --with-zlib=$PREFIX 
make -j8
make install
cd ..


