#!/bin/sh

cd LHG
./build/bin/Release/LHGInterfaceExtractor ../source/include.h -I/opt/local/libexec/llvm-17/lib/clang/17/include -I../../../include
