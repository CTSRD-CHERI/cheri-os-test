#!/bin/bash

source /morello/env/morello-sdk

set -e # Exit on failure

# Download dependencies and build them
CWD=$(pwd)
mkdir -p ext_packages
mkdor -p local
cd ext_packages

# libm
https://libbsd.freedesktop.org/releases/libmd-1.0.4.tar.xz
tar -xf libmd-1.1.0.tar.xz
mkdir -p libmd-1.1.0/build/
cd       libmd-1.1.1/build/
../configure --prefix=$CWD/local/
make
make install

# libxo
wget https://github.com/Juniper/libxo/releases/download/1.7.5/libxo-1.7.5.tar.gz
tar -xf libxo-1.7.5.tar.gz
mkdir -p libxo-1.7.5/build/
cd       libxo-1.7.5/build/
../configure --prefix=$CWD/local/
make -j3
make install

# libbsd
wget https://libbsd.freedesktop.org/releases/libbsd-0.12.2.tar.xz
tar -xf libbsd-0.12.2.tar.xz
mkdir -p libbsd-0.12.2/build/
cd       libbsd-0.12.2/build/
../configure --prefix=$CWD/local/

cd $CWD

# Get the CheriBSD mk files
## FIXME: We seem to have to clone the repository
## just to get the .mk files.
## Is there a nicer way to do this?
git clone --depth=1 --branch=release/25.03 git@github.com:CTSRD-CHERI/cheribsd.git

# Get missing header files
# Musl libc doesn't have a cdefs.h and we are depending on CheriBSD's cdefs.h
mkdir -p $CWD/local/include/sys/
#wget -P  $CWD/local/include/sys/ https://raw.githubusercontent.com/CTSRD-CHERI/cheribsd/refs/tags/release/25.03/sys/sys/cdefs.h
cp $CWD/cheribsd/sys/sys/cdefs/ $CWD/local/include/sys/

# Linux doesn't have linker_set.h
cp $CWD/sys/sys/linker_set.h $CWD/local/include/sys/

# Linux doesn't have nitems(). I put it in utils.h for now
cp $CWD/utils.h $CWD/local/include/utils.h


