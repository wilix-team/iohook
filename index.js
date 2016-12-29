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

  /**
   * Start hook process
   * @param enableLogger Turn on debug logging 
   */
  start(enableLogger) {
    if (this.status == "stopped") {
      NodeHookAddon.startHook(this._handler.bind(this), enableLogger || false);
      this.status = "started";
      this.active = true;
    }
  }

  /**
   * Pause in events call. Just don't fire any new events
   */
  pause() {
    this.active = false;
  }

  /**
   * Resume events call.
   */
  resume() {
    this.active = true;
  }

  /**
   * Shutdown event hook 
   */
  stop() {
    NodeHookAddon.stopHook();
    this.active = false;
    this.status = "stopped";
  }

  /**
   * Local event handler. Don't use it in your code!
   * @param msg Raw event message
   * @private
   */
  _handler(msg) {
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