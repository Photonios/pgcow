#!/usr/bin/env bash

set -e

cd "$(dirname $0)"

data_directory="/opt/pgdata"
pg_version_file="$data_directory/PG_VERSION"
pg_cow_version_file="$data_directory/pgcow.version"

if [[ ! -d "$data_directory" ]]; then
    echo "pgcow: cannot start, $data_directory does not exist"
fi

if [[ ! -f "$pg_version_file" ]]; then
    echo "pgcow: cannot start, data directory $data_directory not initalized"
fi

if [[ ! -f "$pg_cow_version_file" ]]; then
    echo "pgcow: cannot start, data directory $data_directory not initialized for pgcow"
fi

bin/postgres "$@"
