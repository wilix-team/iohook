'use strict';

const ioHook = require('./index.js');

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

console.log('Hook started. Try type something or move mouse');