set shell := ["bash", "-euo", "pipefail", "-c"]

default:
    @just --list

list-examples:
    @find examples/ -mindepth 1 -maxdepth 1 -type d -print0 | xargs -0 -n1 basename | sort

list-examples-json:
    @just list-examples | jq -R -s -c 'split("\n") | map(select(length > 0))'

build-example name:
    pio run -d examples/{{name}}

build-examples:
    @just list-examples | xargs -r -n1 just build-example

format:
    find . \( -name '*.cpp' -o -name '*.h' \) -print0 | xargs -0 clang-format -i

test:
    pio test
