<p align="center">
<img width="20%" src="./contrib/pgcow/logo.png" />
</p>
<br>
<h3 align="center">Bringing copy-on-write semantics to PostgreSQL.</h3>
<br>

---

PGCow is a PostgreSQL distribution in which each database in a cluster is a ZFS dataset. New databases will only start copying from their base template as they are being modified. This allows blazing fast write-able "copies" of large databases with minimal storage usage.

## Code
Most of PGCow resides in an PostgreSQL extension. Some small modifications have been made to PostgreSQL itself to faciliate the extension. These modifications have been contributed upstream. Once merged and released, PGCow can be distributed as an indepdenent extension.

This repository is a copy of the PostgreSQL 11.X release branch (`REL_11_STABLE`). PGCow's extension code has been added to `contrib/pgcow`.

### Upstreamed patches
* Add a hook to the `copydir` function

### Other modifications
* Set default data directory to `/opt/pgdata`
* Load PGCow extension by default
* Allow incoming connections from `0.0.0.0/0`

## Build
Build and packaging instructions below assume that you're running Ubuntu 18.X (Bionic).

1. Install C++ toolchain

        $ sudo add-apt-repository ppa:ubuntu-toolchain-r/test
        $ sudo apt update
        $ sudo apt install -y build-essential pkg-config gcc-9 g++-9 bison flex

2. Install additional libaries for PostgreSQL

        $ sudo apt install -y zlib1g-dev libreadline-dev libxml2-utils xsltproc

3. Install ZFS

        $ sudo apt install -y zfsutils-linux libzfslinux-dev

### Build for distribution

    $ packaging/package.sh

Find the resulting `*.deb` file in `packaging/`.

### Build for local development

    $ CC=gcc-9 CXX=g++-9 ./configure --prefix="$(pwd)/build" --exec-prefix="$(pwd)/build"
    $ make world
    $ make install-world

## Set up
1. Install all required packages

        $ sudo apt install -y zfsutils-linux readline-common  zlib1g libxml2-utils xsltproc

2. Download the `pgcow` Debian package

        $ wget [url]

3. Install the Debian package

        $ sudo dpkg -i [deb file]

1. Create a ZFS data pool in `/opt/pgdata`

        $ sudo zpool create pgdata -o autoexpand=on -o ashift=12 -m /opt/pgdata /dev/xvdb

2. Grant the `postgres` user permissions to mount/clone ZFS datasets

        $ sudo zfs allow postgres clone,snapshot,create,mount,promote pgdata

3. Make `postgres` the owner of the mountpoint

        $ sudo chown postgres:postgres -R /opt/pgdata

4. Initialize the data directory

        $ sudo -u postgres initdb -D /opt/pgdata

5. Initialize PGCow

        $ sudo pgcow-initdb pgdata

6. Enable PGCow to start at boot time

        $ sudo systemctl enable pgcow

7. Start the PGCow service

        $ sudo systemctl start pgcow
