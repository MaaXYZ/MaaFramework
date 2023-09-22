@echo off

set PROTOC=.\MaaDeps\vcpkg\installed\x64-windows\tools\protobuf\protoc
set GRPC_PLUGIN=.\MaaDeps\vcpkg\installed\x64-windows\tools\grpc\grpc_cpp_plugin.exe

set INDIR=.\source\MaaRpc
set OUTDIR=.\source\MaaRpc\generated


set CURSRC=Types.proto
call :BUILD
set CURSRC=Framework.proto
call :BUILD

exit

:BUILD
echo PROCESSING %INDIR%\%CURSRC%
%PROTOC% --proto_path=%INDIR% %INDIR%/%CURSRC% --cpp_out=%OUTDIR%
%PROTOC% --proto_path=%INDIR% %INDIR%/%CURSRC% --grpc_out=%OUTDIR% --plugin=protoc-gen-grpc=%GRPC_PLUGIN%
goto :EOF
