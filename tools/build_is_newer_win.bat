:: compile on my work ThinkPad
@echo off
SETLOCAL
SET NAME=isnewer_win
SET PATH=%PATH%;C:\raylib\w64devkit\bin
SET BUILD_EXE=..\bin\%NAME%.exe
SET GCC_STD=-std=c17
SET INCLUDE_PATHS=-IC:/raylib/w64devkit/include -Iinclude -Isrc
SET LINK_PATHS=-LC:/raylib/w64devkit/lib -Llib
SET LIBS=-lraylib -lgdi32 -lwinmm -lshlwapi
SET GCC_OPTS=%GCC_STD% %INCLUDE_PATHS% %LINK_PATHS% %LIBS% -Wall

gcc ./%NAME%.c -o %BUILD_EXE% %GCC_OPTS%
IF %ERRORLEVEL% NEQ 0 GOTO error

goto end

:error
echo FAILED

:end
ENDLOCAL