@echo off
echo.
echo BUILD START
echo.

SETLOCAL
SET PATH=C:\raylib\w64devkit\bin;%PATH%
SET ISNEWER_SRC=.\tools\isnewer_win.c
SET ISNEWER_EXE=.\bin\isnewer_win.exe
SET BUILDER_SRC=.\tools\build.c
SET BUILD_EXE=.\bin\build.exe
SET ISNEWER_EXE=.\bin\isnewer_win.exe
SET GCC_STD=-std=c17
SET INCLUDE_PATHS=-IC:/raylib/w64devkit/include -Iinclude -Isrc
SET LINK_PATHS=-LC:/raylib/w64devkit/lib -Llib
SET LIBS=-lraylib -lgdi32 -lwinmm -lshlwapi
SET GCC_OPTS=%GCC_STD% %INCLUDE_PATHS% %LINK_PATHS% %LIBS% -Wall

:isnewer
IF EXIST %ISNEWER_EXE% GOTO builder
echo building isnewer
gcc %ISNEWER_SRC% -o %ISNEWER_EXE% %GCC_OPTS%
IF %ERRORLEVEL% NEQ 0 GOTO error

:builder
%ISNEWER_EXE% %BUILDER_SRC% %BUILD_EXE%
IF %ERRORLEVEL% NEQ 0 GOTO buildbuild
%ISNEWER_EXE% .\tools\load_config.h %BUILD_EXE%
IF %ERRORLEVEL% NEQ 0 GOTO buildbuild
GOTO build

:buildbuild
echo build tool out of date. compiling now.
gcc %BUILDER_SRC% -o %BUILD_EXE% %GCC_OPTS%
IF %ERRORLEVEL% NEQ 0 GOTO error

:build
echo running build tool
%BUILD_EXE%
IF %ERRORLEVEL% NEQ 0 GOTO error

echo.
echo BUILD SUCCEEDED
echo.
goto end

:error
echo.
echo BUILD FAILED
echo.

:end
ENDLOCAL