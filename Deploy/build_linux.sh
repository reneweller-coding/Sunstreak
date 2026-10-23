#!/bin/bash
# Builds Sunstreak for Linux, runs the tests and packs dist/Sunstreak-<version>-linux-x86_64.tar.gz (the game, the
# README and an empty music folder for the player's own music, the station MY MUSIC).
#
#   Deploy/build_linux.sh [build directory]      default: ~/sunstreak-build (a Linux file system builds faster)
#
# GLFW needs the X11 development headers (libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev). Where
# they are missing and there is no root (WSL), they come from apt-get download, unpacked into ~/.cache/sunstreak-x11:
# GLFW loads the libraries themselves at run time, so the headers are all it needs.
set -e
SRC="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${1:-$HOME/sunstreak-build}"
VERSION=$(sed -n 's/^project( Sunstreak VERSION \([0-9.]*\).*/\1/p' "$SRC/CMakeLists.txt")
JOBS=$(( $(nproc) < 8 ? $(nproc) : 8 ))

EXTRA=()
if [ ! -f /usr/include/X11/extensions/Xrandr.h ] || [ ! -f /usr/include/X11/extensions/XInput2.h ] ||
   [ ! -f /usr/include/X11/extensions/Xinerama.h ] || [ ! -f /usr/include/X11/Xcursor/Xcursor.h ]; then
    X11="$HOME/.cache/sunstreak-x11"
    if [ ! -f "$X11/usr/include/X11/extensions/XInput2.h" ]; then
        mkdir -p "$X11/debs"
        ( cd "$X11/debs" && apt-get download libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxfixes-dev \
            libxrender-dev libxext-dev )
        for f in "$X11"/debs/*.deb; do dpkg -x "$f" "$X11"; done
    fi
    EXTRA=( "-DCMAKE_PREFIX_PATH=$X11/usr" "-DCMAKE_C_FLAGS=-I$X11/usr/include" "-DCMAKE_CXX_FLAGS=-I$X11/usr/include" )
fi

nice -n 10 cmake -S "$SRC" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DSUNSTREAK_BIN_DIR="$SRC/bin/linux" "${EXTRA[@]}" > /dev/null
nice -n 10 cmake --build "$BUILD" -j "$JOBS" > "$BUILD/build.log" 2>&1 || { grep -E "error" "$BUILD/build.log" | head -40; exit 1; }
"$BUILD/SunstreakTests" > "$BUILD/tests.log" || { cat "$BUILD/tests.log"; exit 1; }
tail -1 "$BUILD/tests.log"

NAME="Sunstreak-$VERSION-linux-x86_64"
STAGE="$BUILD/stage/$NAME"
rm -rf "$BUILD/stage"
mkdir -p "$STAGE"
cp "$BUILD/Sunstreak" "$STAGE/"
strip "$STAGE/Sunstreak"
cp "$SRC/README.md" "$STAGE/"
mkdir -p "$STAGE/music"
echo "Put your own music here (.ogg, .mp3, .flac, .wav, .xm, .mod): it plays as the radio station MY MUSIC." > "$STAGE/music/README.txt"
mkdir -p "$SRC/dist"
tar -C "$BUILD/stage" -czf "$SRC/dist/$NAME.tar.gz" "$NAME"
echo "$SRC/dist/$NAME.tar.gz"
