:: compile on my work ThinkPad
@echo off
SETLOCAL
SET PATH=%PATH%;C:\raylib\w64devkit\bin
SET ISNEWER_SRC=.\tools\isnewer_win.c
SET BUILDER_SRC=.\tools\build.c
SET BUILD_EXE=.\bin\build.exe
SET ISNEWER_EXE=.\bin\isnewer_win.exe
SET ISNEWER_CMD=%ISNEWER_EXE% %BUILDER_SRC% %BUILD_EXE%
SET GCC_STD=-std=c17
SET INCLUDE_PATHS=-IC:/raylib/w64devkit/include -Iinclude -Isrc
SET LINK_PATHS=-LC:/raylib/w64devkit/lib -Llib
SET LIBS=-lraylib -lgdi32 -lwinmm
SET GCC_OPTS=%GCC_STD% %INCLUDE_PATHS% %LINK_PATHS% %LIBS% -Wall

:isnewer
IF EXIST %ISNEWER_EXE% GOTO builder
gcc %ISNEWER_SRC% -o %ISNEWER_EXE% %GCC_OPTS%

:builder
%ISNEWER_CMD%
IF %ERRORLEVEL% == 0 GOTO build

echo build tool out of date. compiling now.
gcc %BUILDER_SRC% -o %BUILD_EXE% %GCC_OPTS%
IF %ERRORLEVEL% NEQ 0 GOTO error

:build
echo running build tool
%BUILD_EXE%
IF %ERRORLEVEL% NEQ 0 GOTO error

goto end

:error
echo FAILED

:end
ENDLOCAL