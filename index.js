'use strict';
const os = require('os');
const EventEmitter = require('events');

let NodeHookAddon;

const OsSystem = os.platform() + '_' + os.arch();
switch (OsSystem) {
  case 'darwin_x64': 
    NodeHookAddon = require("bindings")("iohook.darwin");
    break;
  case 'win32_x64':
    NodeHookAddon = require("bindings")("iohook.win");
    break;
  case 'linux_x64':
    NodeHookAddon = require("bindings")("iohook.linux");
    break;
}

const SegfaultHandler = require('segfault-handler');

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
    this.debug = false;

    SegfaultHandler.registerHandler("iohook-crash.log");
  }

  /**
   * Start hook process
   * @param enableLogger Turn on debug logging 
   */
  start(enableLogger) {
    if (this.status == "stopped") {
      this.debug = enableLogger;
      NodeHookAddon.startHook(this._handler.bind(this), this.debug || false);
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
      console.warn('Unregistered iohook event', msg);
    }
  }
}

module.exports = new IOHook();