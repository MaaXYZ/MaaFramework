#!/bin/sh

FmtVersion=10.1.1
InstallDir=$(dirname $(readlink -f $0))"/../3rdparty/include"
WorkingDir=$(dirname $(readlink -f $0))"/../3rdparty/src/fmt"
echo "FmtVersion: ${FmtVersion}, InstallDir: ${InstallDir}, WorkingDir: ${WorkingDir}"

mkdir -p ${WorkingDir}
wget https://github.com/fmtlib/fmt/releases/download/${FmtVersion}/fmt-${FmtVersion}.zip -O ${WorkingDir}/fmt.zip
unzip ${WorkingDir}/fmt.zip -d ${WorkingDir}
cp -r ${WorkingDir}/fmt-${FmtVersion}/include/fmt ${InstallDir}
