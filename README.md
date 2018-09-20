# iohook

[![Build status](https://ci.appveyor.com/api/projects/status/ph54iicf29ipy8wm?svg=true)](https://ci.appveyor.com/project/WilixLead/iohook)
[![Build Status](https://travis-ci.org/WilixLead/iohook.svg?branch=master)](https://travis-ci.org/WilixLead/iohook)
[![Gitter chat](https://badges.gitter.im/gitterHQ/gitter.png)](https://gitter.im/iohookjs/Lobby)
[![NPM version](https://img.shields.io/npm/v/iohook.svg)](https://www.npmjs.com/package/iohook)

## About
Node.js global native keyboard and mouse listener.

This module can handle keyboard and mouse events via native hooks inside and outside your JavaScript/TypeScript application.

Found a bug? Have an idea? Feel free to post an [issue](https://github.com/WilixLead/iohook/issues) or submit a [PR](https://github.com/WilixLead/iohook/pulls).

**Check out the [documentation](https://wilixlead.github.io/iohook).**

## Installation
iohook provides prebuilt version for a bunch of OSes and runtime versions.

```bash
npm install iohook --save # or yarn add iohook
```

from git

```bash
npm i git+https://github.com:wurikiji/iohook.git 
```

### Windows
- for Node.js usage just make sure that you have installed following components
  - [![](https://cmake.org/wp-content/uploads/2014/06/favicon.png) CMake](https://cmake.org)
  - [![](http://landinghub.visualstudio.com/favicon.ico) Visual C++ Build Tools](http://landinghub.visualstudio.com/visual-cpp-build-tools)
  - [CMake.js](https://www.npmjs.com/package/cmake-js)
  and it will be built by install script
- for Electron go to installation dir (node_modules/iohook) and recompile it according to your Electron version. e.g. `cmake-js compile -r electron -v 2.0.0`

## Added feature

### iohook.enableKeyboardPropagation()

You can enable keyboard event propagation. Keyboard events are propagated by default.

### iohook.disableKeyboardPropagation()

You can disable keyboard event propagtion. Keyboard events are captured and emitted but not propagated to the apps.

## FAQ
Q. *Does this module require Java ?*

A. No, this module doesn't require Java (like jnativehook) or any other runtimes.

## Contributors
Thanks to _kwhat_ for the [libuiohook](https://github.com/kwhat/libuiohook) project and [ayoubserti](https://github.com/ayoubserti) for the first iohook prototype.

* [vespakoen](https://github.com/vespakoen) (prebuild system implementation)
* [matthewshirley](https://github.com/matthewshirley) (Windows prebuild fix)
* [djiit](https://github.com/djiit) (project & community help)
* [ezain](https://github.com/eboukamza) (add feature enable/disable mouse click propagation)
* [wurikiji](https://github.com/wurikiji) (add features. refer to commit log and readme)
* All the other contributors. Feel free to extend this list !
