#!/usr/bin/env bash

set -e

cd "$(dirname $0)"

package_name="pgcow"
package_version="0.0.1~alpha1"
package_dir="$(pwd)/${package_name}_${package_version}"
install_dir="/opt/pgcow"

cpu_count=$(grep -c ^processor /proc/cpuinfo)
