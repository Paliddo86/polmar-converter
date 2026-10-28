@echo off
setlocal

cd /d "%~dp0"
if not exist dist mkdir dist

set COMMON=-std=c++17 -O2 -municode -mwindows -static -static-libgcc -static-libstdc++ -s
set LIBS=-lcomdlg32 -lshell32 -lgdi32 -luser32

g++ %COMMON% polmar_core.cpp main_win.cpp -o dist\polmar-convert-win64.exe %LIBS%
if errorlevel 1 exit /b 1
echo Creato dist\polmar-convert-win64.exe

rem Per la versione 32-bit usare un toolchain MinGW i686-w64-mingw32-g++.
rem i686-w64-mingw32-g++ %COMMON% polmar_core.cpp main_win.cpp -o dist\polmar-convert-win32.exe %LIBS%

endlocal
