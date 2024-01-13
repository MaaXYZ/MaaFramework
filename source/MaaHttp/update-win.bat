cd LHG
.\build\bin\Release\LHGInterfaceExtractor ..\include.h -IC:\Program Files\LLVM\lib\clang\17\include -I..\..\..\include
tsx projects\generator\src\main.ts
