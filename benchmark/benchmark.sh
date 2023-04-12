#!/usr/bin/env bash

# Usage:
#   ./benchmark.sh <ESP IP>

export LC_NUMERIC=C

HOST="$1"
REPEAT=5
CONSUMER_COUNTS="12 10 5 1"
SIZES="10000 5000 1000 500 100 50 10 5 1"

printf "message size\t"
for CONSUMERS in $CONSUMER_COUNTS
do
    printf "%i consumers\t" $CONSUMERS
done
printf "\n"

for SIZE in $SIZES
do
    printf "%i\t" $SIZE
    for CONSUMERS in $CONSUMER_COUNTS
    do
        RESULT=$({
            for ITERATION in $(seq $REPEAT)
            do
                ./benchmark.py --size=$SIZE --consumers=$CONSUMERS "$HOST"
                # potential interference may go away by itself if we wait
                sleep 1
            done
        } | awk '{ sum += $1; count += 1 } END { print sum / count }')

        printf "%.1f\t" $RESULT
    done
    printf "\n"
done
