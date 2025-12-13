#!/bin/bash

INSTALL_PREFIX=$1

if ! [[ -d "$INSTALL_PREFIX" ]]; then
  echo "Usage: $0 [install prefix]"
  exit 1
fi

INSTALL_PREFIX=`realpath "$INSTALL_PREFIX"`

cd `dirname "$0"/..`
PATH="`pwd`/source/MaaUtils/MaaDeps/x-tools/llvm/bin:$PATH"

mkdir -p "$INSTALL_PREFIX/symbols"
find $INSTALL_PREFIX/bin -type f | while read BINARY; do
  echo "Process $BINARY"
  FILE=`basename "$BINARY"`
  DEBUG_FILE="$INSTALL_PREFIX/symbols/$FILE.debug"

  cd `dirname "$BINARY"`

  llvm-objcopy --only-keep-debug "$BINARY" "$BINARY.debug"
  llvm-objcopy --add-gnu-debuglink=$FILE.debug --strip-unneeded "$BINARY"
  mv "$BINARY.debug" "$DEBUG_FILE"

  cd - > /dev/null
done
