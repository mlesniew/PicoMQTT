#!/usr/bin/env bash

if [ "$#" -ne 1 ]
then
	echo "Usage: $0 <pio-board-name>"
	exit 1
fi

set -e

cd "$(dirname "$0")"

mkdir -p examples.build
ROOT_DIR=$PWD

BOARD="$1"
PLATFORM="$(pio boards --json-output | jq -r ".[] | select(.id == \"$BOARD\") | .platform")"

if [ -z "$PLATFORM" ]
then
	echo "Unknown board $BOARD"
	exit 2
fi

find examples -mindepth 1 -type d | cut -d/ -f2 | sort | while read -r EXAMPLE
do
	COMPATIBLE_PLATFORMS="$(grep -hoiE 'platform compatibility:.*' "$ROOT_DIR/examples/$EXAMPLE/"* | cut -d: -f2- | head -n1 )"
	DEPENDENCIES="$(grep -hoiE 'dependencies:.*' "$ROOT_DIR/examples/$EXAMPLE/"* | cut -d: -f2- | head -n1 |sed 's/ /\n\t/g')"
	echo "$EXAMPLE:$COMPATIBLE_PLATFORMS"
	if [ -n "$COMPATIBLE_PLATFORMS" ] && ! echo "$COMPATIBLE_PLATFORMS" | grep -qFiw "$PLATFORM"
	then
		echo "$EXAMPLE: This example is not compatible with this platform"
		continue
	fi

	mkdir -p examples.build/$BOARD/$EXAMPLE
	pushd examples.build/$BOARD/$EXAMPLE
	if [ ! -f platformio.ini ]
	then
		pio init --board=$BOARD
		echo "monitor_speed = 115200" >> platformio.ini
		echo "upload_speed = 921600"  >> platformio.ini
		if [ -n "$DEPENDENCIES" ]
		then
			echo "lib_deps = \t$DEPENDENCIES" >> platformio.ini
		fi
	fi
	ln -s -f -t src/ "$ROOT_DIR/examples/$EXAMPLE/"*
	ln -s -f -t lib/ "$ROOT_DIR"
	if [ -e "$ROOT_DIR/config.h" ]
	then
		ln -s -f -t src/ "$ROOT_DIR/config.h"
	fi
	pio run
	popd
done
