#!/bin/sh

if [ `uname` = Darwin ]; then
  TRIPLET="x64-osx";
else
  TRIPLET="x64-linux";
fi

PROTOC="./MaaDeps/vcpkg/installed/${TRIPLET}/tools/protobuf/protoc"
GRPC_PLUGIN="./MaaDeps/vcpkg/installed/${TRIPLET}/tools/grpc/grpc_cpp_plugin"

INDIR="./include/Interface/proto"
OUTDIR="./source/MaaRpc/generated"

for src in ${INDIR}/*.proto; do
  echo "PROCESSING ${src}"
  ${PROTOC} --proto_path=${INDIR} "${src}" --cpp_out=${OUTDIR}
  ${PROTOC} --proto_path=${INDIR} "${src}" --grpc_out=${OUTDIR} --plugin=protoc-gen-grpc=${GRPC_PLUGIN}
done
