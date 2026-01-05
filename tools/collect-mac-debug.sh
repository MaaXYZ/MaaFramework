#!/bin/bash

INSTALL_PREFIX=$1

if ! [[ -d "$INSTALL_PREFIX" ]]; then
  echo "Usage: $0 [install prefix]"
  exit 1
fi

INSTALL_PREFIX=`realpath "$INSTALL_PREFIX"`

cd `dirname "$0"/..`

mkdir -p "$INSTALL_PREFIX/symbols"
find $INSTALL_PREFIX/bin -type f | while read BINARY; do
  echo "Process $BINARY"
  FILE=`basename "$BINARY"`
  DEBUG_FILE="$INSTALL_PREFIX/symbols/$FILE.dSYM"

  cd `dirname "$BINARY"`

  dsymutil "$BINARY"
  mv "$BINARY.dSYM" "$DEBUG_FILE"

  cd - > /dev/null
done
