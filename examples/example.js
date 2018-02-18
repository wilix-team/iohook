'use strict';

const ioHook = require('../index.js');

ioHook.on("mousedown",function(msg){console.log(msg);});

ioHook.on("keypress",function(msg){console.log(msg);});

ioHook.on("keydown",function(msg){console.log(msg);});

ioHook.on("keyup",function(msg){console.log(msg);});

ioHook.on("mouseclick",function(msg){console.log(msg)});

ioHook.on("mousewheel",function(msg){console.log(msg)});

ioHook.on("mousemove",function(msg){console.log(msg)});

ioHook.on("mousedrag",function(msg){console.log(msg)});

//start ioHook
ioHook.start();
// ioHook.setDebug(true); // Uncomment this line for see all debug information from iohook

const CTRL = 29;
const ALT = 56;
const F7 = 65;

ioHook.registerShortcut([CTRL, F7], (keys) => {
  console.log('Shortcut pressed with keys:', keys);
});

let shId = ioHook.registerShortcut([ALT, F7], (keys) => {
  console.log('This shortcut will be called once. Keys:', keys);
  ioHook.unregisterShortcut(shId);
})

console.log('Hook started. Try type something or move mouse');
