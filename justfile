set shell := ["bash", "-euo", "pipefail", "-c"]
set dotenv-load := true

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

clean:
    rm -rf .pio examples/*/.pio

format:
    find . \( -name '*.cpp' -o -name '*.h' \) -print0 | xargs -0 clang-format -i

test:
    pio test

[script("bash")]
release version:
    set -euo pipefail
    version='{{version}}'

    if ! [[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+([-.][0-9A-Za-z.]+)?$ ]]; then
        echo "Invalid version '$version'. Use e.g. 1.3.1 or 1.3.1-rc1" >&2
        exit 1
    fi

    if ! git diff --quiet || ! git diff --cached --quiet; then
        echo "Working tree is not clean. Commit or stash changes first." >&2
        exit 1
    fi

    if git rev-parse -q --verify "refs/tags/v$version" >/dev/null; then
        echo "Tag v$version already exists." >&2
        exit 1
    fi

    tmp_json="$(mktemp)"
    trap 'rm -f "$tmp_json"' EXIT

    jq --arg version "$version" '.version = $version' library.json > "$tmp_json"
    mv "$tmp_json" library.json
    sed -i "s/^version=.*/version=$version/" library.properties

    git add library.json library.properties
    git commit -m "Release v$version"
    git tag -a "v$version" -m "v$version"

    echo "Created commit and tag v$version"
