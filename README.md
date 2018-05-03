# iohook

[![Build status](https://ci.appveyor.com/api/projects/status/ph54iicf29ipy8wm?svg=true)](https://ci.appveyor.com/project/WilixLead/iohook)
[![Build Status](https://travis-ci.org/WilixLead/iohook.svg?branch=master)](https://travis-ci.org/WilixLead/iohook)
[![Gitter chat](https://badges.gitter.im/gitterHQ/gitter.png)](https://gitter.im/iohookjs/Lobby)

## About

Node.js global native keyboard and mouse listener.

This module can handle keyboard and mouse events via native hooks inside and outside your JavaScript/TypeScript application.

Found a bug ? Have an idea ? Feel free to post an [issue](https://github.com/WilixLead/iohook/issues) or submit a [PR](https://github.com/WilixLead/iohook/pulls).

**Check out the [documentation](#usage) below.**

## OS Support

Every iohook version is built on Linux and Windows. It has been tested on:
- Ubuntu 16.04 / 17.04
- macOS High Sierra 10.13.2 and older
- Windows x32/x64

## Installation

iohook provides prebuilt version for a bunch of OS andruntime versions.

```bash
npm install iohook --save # or yarn add iohook
```

iohook currently provide prebuilt versions for the following runtimes:

- Electron:
  - 1.0.X (ABI 47)
  - 1.2.X (ABI 48)
  - 1.3.X (ABI 49)
  - 1.4.X (ABI 50)
  - 1.5.X (ABI 51)
  - 1.6.X (ABI 53)
  - 1.7.X (ABI 54)
  - 1.8.X (ABI 57)
  - 2.0.X (ABI 57)

- Node.js:
  - 4.6.X (ABI 46)
  - 5.12.X (ABI 47)
  - 6.9.X (ABI 48)
  - 7.4.X (ABI 51)
  - 8.9.X (ABI 57)
  - 9.2.X (ABI 59)
  - 10.0.X (ABI 64)

## Tips

### Usage with Electron

Before installing this module, you will need to set a runtime version.

When developing with webpack, you will need the Node.js runtime. In production, your Electron app will need the Electron version.

Checkout your ABI for [node.js](https://nodejs.org/en/download/releases/) or [electron](https://www.npmjs.com/package/electron-abi). The example below uses Node.js v9.X and Electron v1.8.X.

```json
"iohook": {
  "targets": [
    "node-59",
    "electron-57"
  ],
  "platforms": [
    "win32",
    "darwin",
    "linux"
  ],
  "arches": [
    "x64",
    "ia32"
  ]
}
```

Note: if you use a two-package.json structure, add to application package.json.

## Usage

Here is a simple example :

```javascript
'use strict';
const ioHook = require('iohook');

ioHook.on("mousemove", event => {
  console.log(event);
  /* prints :
    {
      type: 'mousemove',
      x: 700,
      y: 400
    }
  */
});

//Register and start hook
ioHook.start();

// Alternatively, pass true to start in DEBUG mode.
// ioHook.start(true);
```

### Available events

#### keypress (NOT WORKING AT THIS MOMENT, USE keydown/keyup)
Triggered when user presses and releases a key.

```js
{keychar: 'f', keycode: 19, rawcode: 15, type: 'keypress'}
```

#### keydown

Triggered when user presses a key.

```js
{ keychar: 'd', keycode: 46, rawcode: 8, type: 'keydown' }
```

#### keyup

Triggered when user releases a key.

```js
{keychar: 'f', keycode: 19, rawcode: 15, type: 'keyup'}
```

#### mouseclick

Triggered when user clicks a mouse button.
```js
{ button: 1, clicks: 1, x: 545, y: 696, type: 'mouseclick' }
```

#### mousedown

Triggered when user clicks a mouse button.

```js
{ button: 1, clicks: 1, x: 545, y: 696, type: 'mousedown' }
```

#### mouseup

Triggered when user releases a mouse button.

```js
{ button: 1, clicks: 1, x: 545, y: 696, type: 'mouseup' }
```

#### mousemove

Triggered when user moves the mouse.

```js
{ button: 0, clicks: 0, x: 521, y: 737, type: 'mousemove' }
```

#### mousedrag

Triggered when user clicks and drags something.

```js
{ button: 0, clicks: 0, x: 373, y: 683, type: 'mousedrag' }
```

#### mousewheel

Triggered when user uses the mouse wheel.

```js
{ amount: 3, clicks: 1, direction: 3, rotation: 1, type: 'mousewheel', x: 466, y: 683 }
```

### Shortcuts

You can register global shortcuts.  

**NOTE: When a shortcut is caught, keyup/keydown events still emit events. It mean if you register noth keyup AND shortcut for ALT+T, both events will be emited.**

#### registerShortcut(keys, callback)  

In next example we register CTRL+F7 shortcut (in MacOS, for other OS, keycodes can be some different).

```js
let id = ioHook.registerShortcut([29, 65], (keys) => {
  console.log('Shortcut called with keys:', keys)
});
```

#### ioHook.unregisterShortcut(shortcutId)

You can unregister shortcut by using shortcutId returned by `registerShortcut()`.

```js
ioHook.unregisterShortcut(id);
```

#### ioHook.unregisterAllShortcuts()

You can also unregister all shortcuts
```js
ioHook.unregisterAllShortcuts();
```

## Manual Build

If you want to manually compile it, follow the instructions below.

### Ubuntu 16
- `sudo apt install libx11-dev libxtst-dev libxt-dev libx11-xcb-dev libxkbcommon-dev libxkbcommon-x11-dev`
- `npm run build`

### macOS
- `brew install cmake automake libtool pkg-config`
- `npm run build`

### Windows  
- Install: msys2 with autotools, pkg-config, libtool, gcc, clang, glib, C++ Build Tools, cmake  
- `npm run build`

## FAQ

Q. *Does this module require Java ?*

A. No, this module doesn't require Java (like jnativehook) or any other runtimes.

## Contributors

Thanks to _kwhat_ for the [libuiohook](https://github.com/kwhat/libuiohook) project and [ayoubserti](https://github.com/ayoubserti) for the first iohook prototype.

* [vespakoen](https://github.com/vespakoen) (prebuild system implementation)
* [matthewshirley](https://github.com/matthewshirley) (Windows prebuild fix)
* [djiit](https://github.com/djiit) (project & community help)
* All the other contributors. Feel free to extend this list !
