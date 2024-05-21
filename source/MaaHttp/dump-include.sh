#!/bin/bash

INCLUDE_PATHS=$(echo | g++ -E -Wp,-v - 2>&1 | grep '^ /' | tr -d ' ')

INCLUDE_FLAGS=""
for path in $INCLUDE_PATHS; do
    INCLUDE_FLAGS="${INCLUDE_FLAGS} -I${path}"
done

echo "${INCLUDE_FLAGS}"
