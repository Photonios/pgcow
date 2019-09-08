#!/usr/bin/env bash

set -e

source "$(dirname $0)/config.sh"

cd ..

CC=gcc-9 CXX=g++-9 ./configure "--prefix=$package_dir$install_dir" "--exec-prefix=$package_dir$install_dir"
make world -j$(cpu_count)
make install-world

cd "$(dirname $0)"

dpkg-deb --build "$package_dir"
