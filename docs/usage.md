# Usage
## Usage with Electron
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

::: tip
if you use a two-package.json structure, add this to application package.json.
:::

## Usage in a generic Node application
Here is a simple example :

```javascript
'use strict';

const ioHook = require('iohook');

ioHook.on('mousemove', event => {
  console.log(event); // { type: 'mousemove', x: 700, y: 400 }
});

// Register and start hook
ioHook.start();

// Alternatively, pass true to start in DEBUG mode.
ioHook.start(true);
```

## Available events

### keydown

Triggered when user presses a key.

```js
{
  keycode: 46,
  rawcode: 8,
  type: 'keydown',
  altKey: true,
  shiftKey: true,
  ctrlKey: false,
  metaKey: false
}
```

### keyup

Triggered when user releases a key.

```js
{
  keycode: 19,
  rawcode: 15,
  type: 'keyup',
  altKey: true,
  shiftKey: true,
  ctrlKey: false,
  metaKey: false
}
```

### mouseclick

Triggered when user clicks a mouse button.
```js
{ button: 1, clicks: 1, x: 545, y: 696, type: 'mouseclick' }
```

### mousedown

Triggered when user clicks a mouse button.

```js
{ button: 1, clicks: 1, x: 545, y: 696, type: 'mousedown' }
```

### mouseup

Triggered when user releases a mouse button.

```js
{ button: 1, clicks: 1, x: 545, y: 696, type: 'mouseup' }
```

### mousemove

Triggered when user moves the mouse.

```js
{ button: 0, clicks: 0, x: 521, y: 737, type: 'mousemove' }
```

### mousedrag

Triggered when user clicks and drags something.

```js
{ button: 0, clicks: 0, x: 373, y: 683, type: 'mousedrag' }
```

### mousewheel

Triggered when user uses the mouse wheel.

```js
{ amount: 3, clicks: 1, direction: 3, rotation: 1, type: 'mousewheel', x: 466, y: 683 }
```

## Shortcuts

You can register global shortcuts.

::: tip NOTE
When a shortcut is caught, keyup/keydown events still emit events. It means, that if you register a keyup AND shortcut for `ALT+T`, both events will be emited.
:::

### registerShortcut(keys, callback, releaseCallback?)

In the next example we register CTRL+F7 shortcut (in MacOS. For other OSes, the keycodes could be different).

```js
const id = ioHook.registerShortcut([29, 65], (keys) => {
  console.log('Shortcut called with keys:', keys)
});
```

We can also specify a callback to run when our shortcut has been released by specifying a third function argument.

```js
const id = ioHook.registerShortcut([29, 65], (keys) => {
  console.log('Shortcut called with keys:', keys)
}, (keys) => {
  console.log('Shortcut has been released!')
});
```

### unregisterShortcut(shortcutId)

You can unregister shortcut by using shortcutId returned by `registerShortcut()`.

```js
ioHook.unregisterShortcut(id);
```

### unregisterShortcutByKeys(keys)

You can unregister shortcut by using the keys codes passed to `registerShortcut()`. Passing codes in the same order as during registration is not required.

```js
ioHook.unregisterShortcutByKeys(keys);
```

### unregisterAllShortcuts()

You can also unregister all shortcuts.
```js
ioHook.unregisterAllShortcuts();
```

### useRawcode(using)

Some libraries, such as [Mousetrap]() will emit keyboard events that contain
a `rawcode` value. This is a separate, but equally valid, representation of
the key that was pressed. However by default iohook instead uses an event's
`keycode` field to determine which key was pressed. If these key codes do not
line up, your shortcut will not be detected as pressed.

To tell iohook to use the `rawcode` value instead, simply do so before
starting iohook.

```js
iohook.useRawcode(true);
iohook.start();
```

### disableClickPropagation()

You can disable mouse click event propagation. Click events are captured and emitted but not propagated to the window.

```js
ioHook.disableClickPropagation();
```

### enableClickPropagation()

You can enable mouse click event propagation if it's disabled. Click event are propagated by default.

```js
ioHook.enableClickPropagation();
```
