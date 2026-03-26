#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
examples_dir="$script_dir/examples"

for example in "$examples_dir"/*; do
    echo "[build-examples] Building ${example##*/}" >&2
    pio run -d "$example"
done
