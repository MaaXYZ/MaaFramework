#!/bin/sh

if [ `uname` = Darwin ]; then
  TRIPLET="x64-osx";
else
  TRIPLET="x64-linux";
fi

PROTOC="./MaaDeps/vcpkg/installed/${TRIPLET}/tools/protobuf/protoc"
GRPC_PLUGIN="./MaaDeps/vcpkg/installed/${TRIPLET}/tools/grpc/grpc_cpp_plugin"

INDIR="./source/MaaRpc"
OUTDIR="./source/MaaRpc/generated"

function BUILD {
  echo "PROCESSING ${INDIR}/$1"
  ${PROTOC} --proto_path=${INDIR} "${INDIR}/$1" --cpp_out=${OUTDIR}
  ${PROTOC} --proto_path=${INDIR} "${INDIR}/$1" --grpc_out=${OUTDIR} --plugin=protoc-gen-grpc=${GRPC_PLUGIN}
}

BUILD "Types.proto"
BUILD "Framework.proto"
