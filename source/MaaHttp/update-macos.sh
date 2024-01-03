#!/bin/sh

cd LHG
./build/bin/Release/LHGInterfaceExtractor ../include.h -I/opt/local/libexec/llvm-16/lib/clang/16/include -I../../../include
tsx projects/generator/src/main.ts
