'use strict';

var util = require('util');
var EventEmitter = require('events');
var runtime = process.versions['electron'] ? 'electron' : 'node'
var essential = runtime + '-v' + process.versions.modules + '-' + process.platform + '-' + process.arch
console.log('Loading native binary: ./builds/' + essential + '/build/Release/iohook.node')
var NodeHookAddon = require('./builds/' + essential + '/build/Release/iohook.node')

// Try to remove this handler. I hope...
// const SegfaultHandler = require('segfault-handler');

var events = {
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

function IOHook() {
  EventEmitter.call(this, 'IOHook')
  this.status = 'stopped'
  this.active = false
  this.debug = false
}

util.inherits(IOHook, EventEmitter)

IOHook.prototype.start = function(enableLogger) {
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
IOHook.prototype.pause = function() {
  this.active = false;
}

/**
 * Resume events call.
 */
IOHook.prototype.resume = function() {
  this.active = true;
}

/**
 * Shutdown event hook
 */
IOHook.prototype.stop = function() {
  NodeHookAddon.stopHook();
  this.active = false;
  this.status = "stopped";
}

/**
 * Local event handler. Don't use it in your code!
 * @param msg Raw event message
 * @private
 */
IOHook.prototype._handler = function(msg) {
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

module.exports = new IOHook();
