# PGCow

A PostgreSQL distribution introducing copy-on-write semantics for databases.

## About
Most of PGCow resides in an PostgreSQL extension. Some small modifications have been made to PostgreSQL itself to faciliate the extension. These modifications have been contributed upstream. Once merged and released, PGCow can be distributed as an indepdenent extension.

### Source code
PGCow's extension and command line tool's source code lives in `contrib/pgcow`.

### Patches
* Add a hook to the `copydir` function

## Build
Build and packaging instructions below assume that you're running Ubuntu 18.X (Bionic).

1. Install C++ toolchain

        $ sudo add-apt-repository ppa:ubuntu-toolchain-r/test
        $ sudo apt update
        $ sudo apt install -y build-essential pkg-config gcc-9 g++-9 bison flex

2. Install additional libaries for PostgreSQL

        $ sudo apt install -y zlib1g-dev libreadline-dev

3. Install ZFS

        $ sudo apt install -y zfsutils-linux libzfslinux-dev

### Build for distribution

    $ packaging/package.sh

Find the resulting `*.deb` file in `packaging/`.

### Build for local development

    $ CC=gcc-9 CXX=g++-9 ./configure --prefix="$(pwd)/build" --exec-prefix="$(pwd)/build"
    $ make world
    $ make install-world
