#!/usr/bin/env bash

set -e

export PATH="/opt/pgcow/bin:$PATH"
export LD_LIBRARY_PATH="/opt/pgcow/lib:/opt/pgcow/postgresql:$LD_LIBRARY_PATH"
export PKG_CONFIG_PATH="/opt/pgcow/lib/pkgconfig:$PKG_CONFIG_PATH"
