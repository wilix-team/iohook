'use strict';
const os = require('os');
const EventEmitter = require('events');

let runtime = process.versions['electron'] ? 'electron' : 'node';
let essential = runtime + '-v' + process.versions.modules + '-' + process.platform + '-' + process.arch;
console.log('Loading native binary: ./builds/' + essential + '/build/Release/iohook.node');
let NodeHookAddon = require('./builds/' + essential + '/build/Release/iohook.node');

// Try to remove this handler. I hope...
// const SegfaultHandler = require('segfault-handler');

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

    // SegfaultHandler.registerHandler("iohook-crash.log");
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

const iohook = new IOHook();

// Cleanup handler

// do app specific cleaning before exiting
process.on('exit', iohook.stop.bind(iohook));

// catch ctrl+c event and exit normally
process.on('SIGINT', function () {
  iohook.stop();
  // console.log('Ctrl-C...');
  process.exit(2);
});

//catch uncaught exceptions, trace, then exit normally
process.on('uncaughtException', function(e) {
  console.log('Uncaught Exception...');
  console.log(e.stack);
  iohook.stop();
  process.exit(99);
});

module.exports = iohook;
