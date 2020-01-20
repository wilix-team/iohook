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
- `sudo apt install libx11-dev libxtst-dev libxt-dev libx11-xcb-dev libxkbcommon-dev libxkbcommon-x11-dev libpng-dev`
- `npm run build`

## macOS
- `brew install cmake automake libtool pkg-config`
- `npm run build`

## Windows
- Install: msys2 with autotools, pkg-config, libtool, gcc, clang, glib, C++ Build Tools, cmake
- `npm run build`

## Building for specific versions of node

After running `npm run build`, if you want to build iohook for a specific
node/electron version, you can use `build.js` which features the following
command line options:

* `--runtime` specifies whether to build for Electron or plain node
* `--version` specifies what version of Electron/node to build for
* `--abi` specifies what [ABI version](https://nodejs.org/en/docs/guides/abi-stability/) of Electron/node to build against

For example, to build for Electron v4.0.4, you would run:

```
node build.js --runtime electron --version 4.0.4 --abi 69
```

To see more examples of what values to use, view iohook's [package.json file](https://github.com/wilix-team/iohook/blob/master/package.json), under `supportedTargets`. The three values in each block are runtime, version and abi respectively.

`--runtime`, `--version` and `--abi` must all be supplied to build for a specific node version. If they are not supplied, `build.js` will build for the versions specified under `supportedTargets` in your `package.json` (again, see iohook's [package.json file](https://github.com/wilix-team/iohook/blob/master/package.json) for details).

* `--no-upload` tells the script not to attempt to upload the built files to GitHub afterwards

Typically `build.js` is used as part of iohook's CI in order to upload newly-built binaries to NPM. This is thus the default behavior of the script. To prevent this, supply the `--no-upload` flag:

```
node build.js --no-upload
```

# Testing

iohook uses Jest for automated testing. To execute tests, run `npm run test` in your console.

::: warning
It is important you don't press any buttons on your keyboard, don't use your mouse nor the scroll wheel. Tests depend on native events fired by the real keyboard and mouse. Interrupting them will cause tests to fail.
:::
