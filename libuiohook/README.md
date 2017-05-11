libuiohook
==========

A multi-platform C library to provide global keyboard and mouse hooks from userland.

## Compiling
Prerequisites: autotools, pkg-config, libtool, gcc, clang or msys2/mingw32

    ./bootstrap.sh
    ./configure
    make && make install

## Usage
* [Hook Demo](https://github.com/kwhat/libuiohook/blob/master/src/demo_hook.c)
* [Async Hook Demo](https://github.com/kwhat/libuiohook/blob/master/src/demo_hook_async.c)
* [Event Post Demo](https://github.com/kwhat/libuiohook/blob/master/src/demo_post.c)
* [Properties Demo](https://github.com/kwhat/libuiohook/blob/master/src/demo_properties.c)
* [Public Interface](https://github.com/kwhat/libuiohook/blob/master/include/uiohook.h)
* Please see the man pages for function documentation.
