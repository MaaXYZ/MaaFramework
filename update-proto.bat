@echo off

set PROTOC=.\MaaDeps\vcpkg\installed\x64-windows\tools\protobuf\protoc
set GRPC_PLUGIN=.\MaaDeps\vcpkg\installed\x64-windows\tools\grpc\grpc_cpp_plugin.exe

set INDIR=.\include\MaaFramework\interfaces
set OUTDIR=.\source\MaaRpc\generated

for %%s in (%INDIR%\*.proto) do (
  echo PROCESSING %%s
  %PROTOC% --proto_path=%INDIR% %%s --cpp_out=%OUTDIR%
  %PROTOC% --proto_path=%INDIR% %%s --grpc_out=%OUTDIR% --plugin=protoc-gen-grpc=%GRPC_PLUGIN%
)
