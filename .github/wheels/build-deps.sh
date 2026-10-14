#!/bin/bash
# Build the C/C++ dependencies of the cadabra2 Python module inside the
# manylinux container used by cibuildwheel (CIBW_BEFORE_ALL). Both GMP
# and Boost are built as static libraries with -fPIC, and installed in
# /usr/local, so that they get linked into the module and the wheel
# does not depend on any shared libraries outside the base system.

set -e

GMP_VERSION=6.3.0
BOOST_VERSION=1.86.0
BOOST_DIR=boost_${BOOST_VERSION//./_}

dnf install -y m4 xz wget

cd /tmp

# GMP (with the C++ bindings, gmpxx).
wget -q https://ftp.gnu.org/gnu/gmp/gmp-${GMP_VERSION}.tar.xz
tar xf gmp-${GMP_VERSION}.tar.xz
cd gmp-${GMP_VERSION}
./configure --prefix=/usr/local --enable-cxx --enable-static --disable-shared --with-pic
make -j"$(nproc)"
make install
cd /tmp

# Boost: headers plus a static Boost.Filesystem (the only compiled
# Boost library the module links against).
wget -q https://archives.boost.io/release/${BOOST_VERSION}/source/${BOOST_DIR}.tar.gz
tar xzf ${BOOST_DIR}.tar.gz
cd ${BOOST_DIR}
./bootstrap.sh --prefix=/usr/local --with-libraries=filesystem
./b2 -j"$(nproc)" link=static runtime-link=shared cxxflags=-fPIC variant=release install
cd /tmp

rm -rf gmp-${GMP_VERSION}* ${BOOST_DIR}*
