# Manual Build

If you want to manually compile it, follow the instructions below.

## Ubuntu 16
- `sudo apt install libx11-dev libxtst-dev libxt-dev libx11-xcb-dev libxkbcommon-dev libxkbcommon-x11-dev`
- `npm run build`

## macOS
- `brew install cmake automake libtool pkg-config`
- `npm run build`

## Windows
- Install: msys2 with autotools, pkg-config, libtool, gcc, clang, glib, C++ Build Tools, cmake
- `npm run build`
