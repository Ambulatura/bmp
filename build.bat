@echo off

IF NOT EXIST build mkdir build
pushd build

call vcvarsall.bat x64

REM 64-bit build.

REM /W4 /WX

cl /MTd /Zi /FC /nologo ..\main.cpp


popd

