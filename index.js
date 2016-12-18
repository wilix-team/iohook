'use strict';

const NodeHookAddon = require("bindings")("iohook");
const EventEmitter = require('events');

const SegfaultHandler = require('segfault-handler');

SegfaultHandler.registerHandler("crash.log"); // With no argument, SegfaultHandler will generate a generic log file name

// Optionally specify a callback function for custom logging. This feature is currently only supported for Node.js >= v0.12 running on Linux.
SegfaultHandler.registerHandler("crash.log", function(signal, address, stack) {
  console.log(arguments);
});

const events = {
  3: 'keypress',
  4: 'keydown',
  5: 'keyup',
  6: 'mouseclick',
  7: 'mousedown',
  8: 'mouseup',
  9: 'mousemove',
  10: 'mousedrag',
  11: 'mousewheel'
};

class IOHook extends EventEmitter {
  constructor() {
    super();
    this.status = "stopped";
    this.active = false;
  }

  start() {
    if (this.status == "stopped") {
      NodeHookAddon.startHook(this.handler.bind(this));
      this.status = "started";
      this.active = true;
    }
  }

  pause() {
    this.active = false;
  }

  resume() {
    this.active = true;
  }

  stop() {
    NodeHookAddon.stopHook();
    this.active = false;
    this.status = "stopped";
  }

  handler(msg) {
    if (this.active == false) {
      return;
    }
    
    if (!msg) {
      return;
    }

    if (events[msg.type]) {
      let event = msg.mouse || msg.keyboard || msg.wheel;
      event.type = events[msg.type];
      this.emit(events[msg.type], event);
    } else {
      console.debug('Unregistered iohook event', msg);
    }
  }
}

module.exports = new IOHook();