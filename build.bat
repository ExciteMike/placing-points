:: compile on my work ThinkPad
@echo off
SETLOCAL
SET PATH=%PATH%;C:\raylib\w64devkit\bin
SET BUILD_EXE=.\bin\build.exe
SET GCC_STD=-std=c17
SET INCLUDE_PATHS=-IC:/raylib/w64devkit/include -Iinclude -Isrc
SET LINK_PATHS=-LC:/raylib/w64devkit/lib -Llib
SET LIBS=-lraylib -lgdi32 -lwinmm
SET GCC_OPTS=%GCC_STD% %INCLUDE_PATHS% %LINK_PATHS% %LIBS% -Wall

:: bootstrap build program
IF EXIST %BUILD_EXE% GOTO build_exe
GOTO build

:build_exe
gcc ./script/build.c -o %BUILD_EXE% %GCC_OPTS%
IF ERRORLEVEL GOTO error

:build
%BUILD_EXE%

goto end

:error
echo FAILED

:end
ENDLOCAL