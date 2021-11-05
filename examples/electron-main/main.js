'use strict';

const { app } = require('electron');
const ioHook = require('iohook');

function eventHandler(event) {
  console.log(event);
}

app.on('ready', () => {
  console.log(
    'node: ' +
      process.versions.node +
      ', chromium: ' +
      process.versions.chrome +
      ', electron: ' +
      process.versions.electron
  );
  ioHook.start(true);
  ioHook.on('mouseclick', eventHandler);
  ioHook.on('keydown', eventHandler);
  ioHook.on('mousewheel', eventHandler);
  ioHook.on('mousemove', eventHandler);
  console.log('Try move your mouse or press any key');
});

app.on('before-quit', () => {
  ioHook.unload();
  ioHook.stop();
});
