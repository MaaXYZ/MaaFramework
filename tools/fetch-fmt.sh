#!/bin/sh

FmtVersion=10.1.1
BaseDir=$(dirname "$(readlink -f $0)")
InstallDir=${BaseDir}/../3rdparty/include
WorkingDir=${BaseDir}/../3rdparty/src/fmt
echo "FmtVersion: ${FmtVersion}, InstallDir: ${InstallDir}, WorkingDir: ${WorkingDir}"

mkdir -p "${WorkingDir}"
wget https://github.com/fmtlib/fmt/releases/download/${FmtVersion}/fmt-${FmtVersion}.zip -O "${WorkingDir}/fmt.zip"
unzip "${WorkingDir}/fmt.zip" -d "${WorkingDir}"
cp -r "${WorkingDir}/fmt-${FmtVersion}/include/fmt" "${InstallDir}"
