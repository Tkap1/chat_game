
@echo off

cls

where /q cl.exe
if ERRORLEVEL 1 (
	call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
)

if not exist build\NUL mkdir build

set exe_name=main.exe
set linker=-link user32.lib gdi32.lib opengl32.lib
set compiler=-nologo -W4 -wd4505 -wd4100 -wd4324 -wd4201 -MT -Zi -Dm_game -FC -Od -Zi -D_CRT_SECURE_NO_WARNINGS -std:c++20 -Zc:strictStrings- -Dm_debug
set files=..\src\main.cpp

pushd build
	cl %files% %compiler% /Fe:%exe_name% %linker%
popd
copy /Y build\%exe_name% %exe_name% > NUL