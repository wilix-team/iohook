# Manual Build

::: tip INFO
This is not required for regular users. You should follow this page only if you want to build the source files yourself.
:::

::: tip WARNING
When you run `npm run build`, it will try to download a prebuilt for your platform/target, and sometime fail if you are building for a recent target. You can safely ignore this step and the associated warning.
:::

You can build own modules directly in your node_modules under the project. Go to ~/my_project/node_modules/iohook path via terminal.
Or you can just clone iohook repository and working on it, then copy binnary files to project's node_modules or change download path in iohook and make you own version of module.

Before start, you need install required dependencies for build:

## Linux

- `sudo apt-get install -y libx11-dev libx11-xcb-dev libxkbcommon-dev libxkbcommon-x11-dev`
- `sudo apt-get install libxtst-dev libpng++-dev`
  - These dependencies belong to [robotjs]. You would only need them if there is no `robotjs` prebuilt for your platform. If so, the `npm install` command will fail without these dependencies.
- `npm install`
- `npm run build`

## macOS

- Install: Xcode Command Line Tools. It is required for `robotjs`
- `npm install`
- `npm run build`

## Windows

- Install: `msys2` with `autotools`, `pkg-config`, `libtool`, `gcc`, `clang`, `glib`, `C++ Build Tools`
- `npm install`
- `npm run build`

## Building for specific versions of node

Running `npm run build` will detect your platform and build into `./builds`. You can also use `build.js` which features the following
command line arguments:

- `--runtime` specifies whether to build for Electron or plain node.
- `--version` specifies which version of Electron/node to build for.
- `--abi` specifies which [ABI version](https://nodejs.org/en/docs/guides/abi-stability/) of Electron/node to build against.

For example, to build for Electron v4.0.4, you would run:

```
node build.js --runtime electron --version 4.0.4 --abi 69
```

To see more examples of what values to use, view iohook's [package.json file](https://github.com/wilix-team/iohook/blob/master/package.json), under `supportedTargets`. The three values in each block are runtime, version and abi respectively.

`--runtime`, `--version` and `--abi` must all be supplied to build for a specific node version. If they are not supplied, `build.js` will build for the versions specified under `supportedTargets` in your `package.json` (again, see iohook's [package.json file](https://github.com/wilix-team/iohook/blob/master/package.json) for details).

- `--upload=false` tells the script not to attempt to upload the built files to GitHub afterwards.

- `--all` tells the script to build all supported targets. Useful for CI.

Typically `build.js` is used as part of iohook's CI in order to upload newly-built binaries to NPM. This is thus the default behavior of the script. To prevent this, supply the `--upload=false` flag:

```
node build.js --upload=false
```

# Testing

iohook uses Jest for automated testing. To execute tests, run `npm run test` in your console.

::: WARNING
It is important you don't press any buttons on your keyboard, don't use your mouse, or the scroll wheel. Tests depend on native events fired by the real keyboard and mouse. Interrupting them will cause tests to fail.
:::
