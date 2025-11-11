#!/bin/bash

set -e # Exit on failure
set -x

# Download dependencies and build them
CWD=$(pwd)
mkdir -p ext_packages
mkdir -p local

# Get CheriBSD
# We need the CheriBSD .mk files and we take bmake from it
## FIXME: We seem to have to clone the repository
## just to get the .mk files.
## Is there a nicer way to do this?
if [ ! -d "cheribsd" ]; then
  git clone --depth=1 --branch=release/25.03 https://github.com/CTSRD-CHERI/cheribsd.git
fi

# bmake
cp -r $CWD/cheribsd/contrib/bmake/ .
cd bmake
./boot-strap --prefix=$CWD/local/ >> log
./configure >> log
make -j3 >> log
make install >> log

# Libraries that we use need to be purecap
source /morello/env/morello-sdk

# libm
wget https://libbsd.freedesktop.org/releases/libmd-1.1.0.tar.xz >> log
tar -xf libmd-1.1.0.tar.xz >> log
mkdir -p libmd-1.1.0/build/
cd       libmd-1.1.0/build/
../configure --prefix=$CWD/local/ >> log
make -j3 >> log
make install >> log

# libxo
wget https://github.com/Juniper/libxo/releases/download/1.7.5/libxo-1.7.5.tar.gz >> log
tar -xf libxo-1.7.5.tar.gz >> log
mkdir -p libxo-1.7.5/build/
cd       libxo-1.7.5/build/
../configure --prefix=$CWD/local/
make -j3 >> log
make install >> log

# libbsd
wget https://libbsd.freedesktop.org/releases/libbsd-0.12.2.tar.xz >> log
tar -xf libbsd-0.12.2.tar.xz >> log
mkdir -p libbsd-0.12.2/build/
cd       libbsd-0.12.2/build/
../configure --prefix=$CWD/local/ >> log
make -j3 >> log
make install >> log

cd $CWD

# Get missing header files
# Musl libc doesn't have a cdefs.h and we are depending on CheriBSD's cdefs.h
mkdir -p $CWD/local/include/sys/
#wget -P  $CWD/local/include/sys/ https://raw.githubusercontent.com/CTSRD-CHERI/cheribsd/refs/tags/release/25.03/sys/sys/cdefs.h
cp $CWD/cheribsd/sys/sys/cdefs.h $CWD/local/include/sys/

# Linux doesn't have linker_set.h
cp $CWD/cheribsd/sys/sys/linker_set.h $CWD/local/include/sys/

# Copy armreg.h
mkdir -p $CWD/local/include/machine/
cp $CWD/cheribsd/sys/arm64/include/armreg.h $CWD/local/include/machine/


