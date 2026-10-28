#!/bin/sh
set -eu
cd "$(dirname "$0")"
mkdir -p dist

COMMON="-std=c++17 -O2 -municode -mwindows -static -static-libgcc -static-libstdc++ -s"
LIBS="-lcomdlg32 -lshell32 -lgdi32 -luser32"

built=0

if command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
    x86_64-w64-mingw32-g++ $COMMON polmar_core.cpp main_win.cpp \
        -o dist/polmar-convert-win64.exe $LIBS
    echo "Creato: $(pwd)/dist/polmar-convert-win64.exe"
    built=1
fi

if command -v i686-w64-mingw32-g++ >/dev/null 2>&1; then
    i686-w64-mingw32-g++ $COMMON polmar_core.cpp main_win.cpp \
        -o dist/polmar-convert-win32.exe $LIBS
    echo "Creato: $(pwd)/dist/polmar-convert-win32.exe"
    built=1
fi

if [ "$built" -eq 0 ]; then
    if ! command -v g++ >/dev/null 2>&1; then
        echo "Errore: nessun compilatore MinGW-w64 trovato." >&2
        echo "Installa mingw-w64 oppure usa un terminale MSYS2 UCRT64/MINGW32." >&2
        exit 1
    fi

    machine=$(g++ -dumpmachine)
    case "$machine" in
        x86_64*)
            out=polmar-convert-win64.exe
            ;;
        i686*|i386*)
            out=polmar-convert-win32.exe
            ;;
        *)
            echo "Errore: g++ non e' un cross-compiler Windows ($machine)." >&2
            exit 1
            ;;
    esac

    g++ $COMMON polmar_core.cpp main_win.cpp -o "dist/$out" $LIBS
    echo "Creato: $(pwd)/dist/$out"
fi
