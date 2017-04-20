'use strict';
const os = require('os');
const EventEmitter = require('events');
const path = require('path');

const runtime = process.versions['electron'] ? 'electron' : 'node';
const essential = runtime + '-v' + process.versions.modules + '-' + process.platform + '-' + process.arch;
const modulePath = path.join(__dirname, 'builds', essential, 'build', 'Release', 'iohook.node');
console.log('Loading native binary:', modulePath);
let NodeHookAddon = require(modulePath);

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
    this.active = false;
    
    this.load();
    this.setDebug(false);
  }

  /**
   * Start hook process
   * @param enableLogger Turn on debug logging 
   */
  start(enableLogger) {
    if (!this.active) {
      this.active = true;
      this.setDebug(enableLogger);
    }
  }

  /**
   * Shutdown event hook 
   */
  stop() {
    if (this.active) {
      this.active = false;
    }
  }

  /**
   * Load native module
   */
  load() {
    NodeHookAddon.startHook(this._handler.bind(this), this.debug || false);
  }
  
  /**
   * Unload native module and stop hook
   */
  unload() {
    this.stop();
    NodeHookAddon.stopHook();
  }

  /**
   * Enable or disable native debug output
   * @param {Boolean} mode
   */
  setDebug(mode) {
    NodeHookAddon.debugEnable(mode ? true : false);
  }
  
  /**
   * Local event handler. Don't use it in your code!
   * @param msg Raw event message
   * @private
   */
  _handler(msg) {
    if (this.active === false) {
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
process.on('exit', iohook.unload.bind(iohook));

// catch ctrl+c event and exit normally
process.on('SIGINT', function () {
  iohook.unload();
  // console.log('Ctrl-C...');
  process.exit(2);
});

//catch uncaught exceptions, trace, then exit normally
process.on('uncaughtException', function(e) {
  console.log('Uncaught Exception...');
  console.log(e.stack);
  iohook.unload();
  process.exit(99);
});

module.exports = iohook;
