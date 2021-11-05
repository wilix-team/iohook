# iohook

[![NPM version](https://img.shields.io/npm/v/iohook?color=%230088FF)](https://www.npmjs.com/package/iohook)
[![Release date](https://img.shields.io/github/release-date/wilix-team/iohook?color=%230088FF)](https://github.com/wilix-team/iohook/releases/latest)
[![GitHub Super-Linter](https://github.com/wilix-team/iohook/workflows/Lint%20Code%20Base/badge.svg)](https://github.com/marketplace/actions/super-linter)
[![CI](https://github.com/wilix-team/iohook/actions/workflows/ci.yml/badge.svg)](https://github.com/wilix-team/iohook/actions/workflows/ci.yml)
[![code style: prettier](https://img.shields.io/badge/code_style-prettier-ff69b4.svg?color=%23008880)](https://github.com/prettier/prettier)
[![Gitter chat](https://badges.gitter.im/gitterHQ/gitter.png)](https://gitter.im/iohookjs/Lobby)
[![Issues](https://img.shields.io/github/issues-raw/wilix-team/iohook)](https://github.com/wilix-team/iohook/issues)

## About

Node.js global native keyboard and mouse listener.

This module can handle keyboard and mouse events via native hooks inside and outside your JavaScript/TypeScript application.

Found a bug? Have an idea? Feel free to post an [issue](https://github.com/wilix-team/iohook/issues) or submit a [PR](https://github.com/wilix-team/iohook/pulls).

**Check out the [documentation](https://wilix-team.github.io/iohook).**

## Platform support

- Versions >= 0.6.0 support only officially supported platforms versions.
- Versions 0.5.X are the last to support Electron < 4.0.0
- Versions 0.4.X are the last to support for Node < 8.0 and Electron < 2.0.0

## Installation

iohook provides prebuilt version for a bunch of OSes and platforms.

### Linux (including WSL)

```bash
# On Linux (including WSL) platform, you will need libxkbcommon-x11 installed
sudo apt-get install -y libxkbcommon-x11-0
```

### All platforms

```bash
npm install iohook --save # or yarn add iohook
```

## FAQ

Q. _Does this module require Java ?_

A. No, this module doesn't require Java (like jnativehook) or any other runtimes.

Q. _Is iohook compatible with Node/Electron version X.Y.Z ?_

A. We try to match the currently supported version of both [Node](https://nodejs.org/en/about/releases/) and [Electron](https://electronjs.org/docs/tutorial/support#currently-supported-versions).

## Apps

Are you using iohook in your project ? Please tell us in a [PR](https://github.com/wilix-team/iohook/pulls) so we an add it to the list !

- [Cortex](https://crtx.gg/)
- [Tracklify](https://tracklify.com/)
- [CrewLink](https://github.com/ottomated/CrewLink)
- [Runtime](https://github.com/yikuansun/desktopspeedruntools#runtime-speedrun-tools)

## Contributors

Thanks to _kwhat_ for the [libuiohook](https://github.com/kwhat/libuiohook) project and [ayoubserti](https://github.com/ayoubserti) for the first iohook prototype.

- [vespakoen](https://github.com/vespakoen) (prebuild system implementation)
- [matthewshirley](https://github.com/matthewshirley) (Windows prebuild fix)
- [djiit](https://github.com/djiit) (project & community help)
- [ezain](https://github.com/eboukamza) (add feature enable/disable mouse click propagation)
- [anoadragon453](https://github.com/anoadragon453) (electron 4+ support)
- [ykhwong](https://github.com/ykhwong) (node-gyp usage, electron 9+ support)
- All the other contributors. Feel free to extend this list !
