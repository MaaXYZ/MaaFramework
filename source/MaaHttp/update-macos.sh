#!/bin/sh

cd LHG
./build/bin/Release/LHGInterfaceExtractor ../include.h -I/opt/local/libexec/llvm-17/lib/clang/17/include -I../../../include
tsx projects/generator/src/new.ts
