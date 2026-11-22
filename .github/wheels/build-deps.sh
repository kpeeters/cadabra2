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
# Update these together with the versions above.
GMP_SHA256=a3c2b80201b89e68616f4ad30bc66aee4927c3ce50e33929ca819d5c43538898
BOOST_SHA256=2575e74ffc3ef1cd0babac2c1ee8bdb5782a0ee672b1912da40e5b4b591ca01f

dnf install -y m4 xz wget

# Download a file and check its sha256, trying each of the given URLs
# in turn (ftp.gnu.org in particular regularly hangs or refuses
# connections from CI runners).
fetch() {
    local file=$1 sha=$2
    shift 2
    for url in "$@"; do
        echo "Downloading ${file} from ${url}"
        if wget --tries=3 --timeout=60 -nv -O "${file}" "${url}" \
              && echo "${sha}  ${file}" | sha256sum -c -; then
            return 0
        fi
        echo "Download of ${file} from ${url} failed"
        rm -f "${file}"
    done
    echo "Could not download ${file}"
    exit 1
}

cd /tmp

# GMP (with the C++ bindings, gmpxx).
fetch gmp-${GMP_VERSION}.tar.xz ${GMP_SHA256} \
      https://ftpmirror.gnu.org/gmp/gmp-${GMP_VERSION}.tar.xz \
      https://gmplib.org/download/gmp/gmp-${GMP_VERSION}.tar.xz \
      https://ftp.gnu.org/gnu/gmp/gmp-${GMP_VERSION}.tar.xz
tar xf gmp-${GMP_VERSION}.tar.xz
cd gmp-${GMP_VERSION}
./configure --prefix=/usr/local --enable-cxx --enable-static --disable-shared --with-pic
make -j"$(nproc)"
make install
cd /tmp

# Boost: headers plus a static Boost.Filesystem (the only compiled
# Boost library the module links against).
fetch ${BOOST_DIR}.tar.gz ${BOOST_SHA256} \
      https://archives.boost.io/release/${BOOST_VERSION}/source/${BOOST_DIR}.tar.gz \
      https://sourceforge.net/projects/boost/files/boost/${BOOST_VERSION}/${BOOST_DIR}.tar.gz/download
tar xzf ${BOOST_DIR}.tar.gz
cd ${BOOST_DIR}
./bootstrap.sh --prefix=/usr/local --with-libraries=filesystem
./b2 -j"$(nproc)" link=static runtime-link=shared cxxflags=-fPIC variant=release install
cd /tmp

rm -rf gmp-${GMP_VERSION}* ${BOOST_DIR}*
