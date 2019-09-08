#!/usr/bin/env bash

set -e

source "$(dirname $0)/config.sh"

rm -f *.deb
rm -rf "$package_dir$install_dir/*"
