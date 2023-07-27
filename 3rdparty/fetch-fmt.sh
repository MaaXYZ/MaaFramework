#!/bin/sh

mkdir -p 3rdparty/src
wget 'https://github.com/fmtlib/fmt/releases/download/10.0.0/fmt-10.0.0.zip' -O 3rdparty/src/fmt.zip
unzip 3rdparty/src/fmt.zip -d 3rdparty/src
cp -r 3rdparty/src/fmt-10.0.0/include/fmt 3rdparty/include
