'use strict';

const {app} = require('electron');
const ioHook = require('../../index');

function eventHandler(event) {
  console.log(event);
}

app.on('ready', () => {
  ioHook.start(true);
  ioHook.on('mouseclick', eventHandler);
  ioHook.on('keypress', eventHandler);
  ioHook.on('mousewheel', eventHandler);
  ioHook.on('mousemove', eventHandler);
  console.log('Try move your mouse or press any key');
});

app.on('before-quit', () => {
  ioHook.unload();
  ioHook.stop();
});
