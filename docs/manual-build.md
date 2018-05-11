# Manual Build

::: tip INFO
This is not required for regular users. You should follow this page only if you want to build the source files yourself.
:::

Firstly, run this script
```js
const path = require('path');
const runtime = process.versions['electron'] ? 'electron' : 'node';
const essential = runtime + '-v' + process.versions.modules + '-' + process.platform + '-' + process.arch;
const modulePath = path.join(__dirname, 'builds', essential, 'build', 'Release', 'iohook.node');
console.info('The path is:', modulePath);
```
and get the full path, to where the package looks for the binary. (It's enough to run `node index.js` and look for result of the `console.info` in line `15`).
Then, run `npm run build` and copy created binary `iohook.node` file from `build/Release/iohook.node` to the `console.info`'ed path.
Running `node examples/example.js` should show you a working result of your build.
For build requirements for your OS look below.

## Ubuntu 16
- `sudo apt install libx11-dev libxtst-dev libxt-dev libx11-xcb-dev libxkbcommon-dev libxkbcommon-x11-dev`
- `npm run build`

## macOS
- `brew install cmake automake libtool pkg-config`
- `npm run build`

## Windows
- Install: msys2 with autotools, pkg-config, libtool, gcc, clang, glib, C++ Build Tools, cmake
- `npm run build`
