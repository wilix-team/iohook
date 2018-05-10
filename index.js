const EventEmitter = require('events');
const path = require('path');

// Try use handler if runtime and ABI is compatible
try {
  const SegfaultHandler = require('segfault-handler');
  SegfaultHandler.registerHandler("iohook-crash.log");
} catch (e) {}

const runtime = process.versions['electron'] ? 'electron' : 'node';
const essential = runtime + '-v' + process.versions.modules + '-' + process.platform + '-' + process.arch;
const modulePath = path.join(__dirname, 'builds', essential, 'build', 'Release', 'iohook.node');
if (process.env.DEBUG) {
  console.info('Loading native binary:', modulePath);
}
let NodeHookAddon = require(modulePath);

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
    this.shortcuts = [];

    this.lastKeydownShift = false;
    this.lastKeydownAlt = false;
    this.lastKeydownCtrl = false;
    this.lastKeydownMeta = false;

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
   * Register global shortcut. When all keys in keys array pressed, callback will be called
   * @param {Array} keys Array of keycodes
   * @param {Function} callback Callback for call when shortcut pressed
   * @return {number} ShortcutId for unregister
   */
  registerShortcut(keys, callback) {
    let shortcut = {};
    let shortcutId = Date.now() + Math.random();
    keys.forEach(keyCode => {
      shortcut[keyCode] = false;
    })
    shortcut.id = shortcutId;
    shortcut.callback = callback;
    this.shortcuts.push(shortcut);
    return shortcutId;
  }

  /**
   * Unregister shortcut by ShortcutId
   * @param shortcutId
   */
  unregisterShortcut(shortcutId) {
    this.shortcuts.forEach((shortcut,i) => {
      if (shortcut.id === shortcutId) {
        this.shortcuts.splice(i, 1);
      }
    });
  }

  /**
   * Unregister all shortcuts
   */
  unregisterAllShortcuts() {
    this.shortcuts.splice(0, this.shortcuts.length);
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
    NodeHookAddon.debugEnable(mode);
  }

  /**
   * Disable mouse click propagation.
   * The click event are captured and the event emitted but not propagated to the window.
   */
  disableClickPropagation() {
    NodeHookAddon.grabMouseClick(true);
  }

  /**
   * Enable mouse click propagation (enabled by default).
   * The click event are emitted and propagated.
   */
  enableClickPropagation() {
    NodeHookAddon.grabMouseClick(false);
  }

  /**
   * Local event handler. Don't use it in your code!
   * @param msg Raw event message
   * @private
   */
  _handler(msg) {
    if (this.active === false || !msg) return;

    if (events[msg.type]) {
      const event = msg.mouse || msg.keyboard || msg.wheel;

      event.type = events[msg.type];

      this._handleShift(event);
      this._handleAlt(event);
      this._handleCtrl(event);
      this._handleMeta(event);

      this.emit(events[msg.type], event);

      // If there is any registered shortcuts then handle them.
      if ((event.type === 'keydown' || event.type === 'keyup') && iohook.shortcuts.length > 0) {
        this._handleShortcut(event);
      }
    }
  }

  /**
   * Handles the shift key. Whenever shift is pressed, all future events would
   * contain { shiftKey: true } in its object, until the shift key is released.
   * @param event Event object
   * @private
   */
  _handleShift(event) {
    if (event.type === 'keyup' && event.shiftKey) {
      this.lastKeydownShift = false;
    }

    if (event.type === 'keydown' && event.shiftKey) {
      this.lastKeydownShift = true;
    }

    if (this.lastKeydownShift) {
      event.shiftKey = true;
    }
  }

  /**
   * Handles the alt key. Whenever alt is pressed, all future events would
   * contain { altKey: true } in its object, until the alt key is released.
   * @param event Event object
   * @private
   */
  _handleAlt(event) {
    if (event.type === 'keyup' && event.altKey) {
      this.lastKeydownAlt = false;
    }

    if (event.type === 'keydown' && event.altKey) {
      this.lastKeydownAlt = true;
    }

    if (this.lastKeydownAlt) {
      event.altKey = true;
    }
  }

  /**
   * Handles the ctrl key. Whenever ctrl is pressed, all future events would
   * contain { ctrlKey: true } in its object, until the ctrl key is released.
   * @param event Event object
   * @private
   */
  _handleCtrl(event) {
    if (event.type === 'keyup' && event.ctrlKey) {
      this.lastKeydownCtrl = false;
    }

    if (event.type === 'keydown' && event.ctrlKey) {
      this.lastKeydownCtrl = true;
    }

    if (this.lastKeydownCtrl) {
      event.ctrlKey = true;
    }
  }

  /**
   * Handles the meta key. Whenever meta is pressed, all future events would
   * contain { metaKey: true } in its object, until the meta key is released.
   * @param event Event object
   * @private
   */
  _handleMeta(event) {
    if (event.type === 'keyup' && event.metaKey) {
      this.lastKeydownMeta = false;
    }

    if (event.type === 'keydown' && event.metaKey) {
      this.lastKeydownMeta = true;
    }

    if (this.lastKeydownMeta) {
      event.metaKey = true;
    }
  }

  /**
   * Local shortcut event handler
   * @param event Event object
   * @private
   */
  _handleShortcut(event) {
    if (this.active === false) {
      return;
    }
    if (event.type === 'keydown') {
      this.shortcuts.forEach(shortcut => {
        if (shortcut[event.keycode] !== undefined) {
          shortcut[event.keycode] = true;

          let keysTmpArray = [];
          let callme = true;
          Object.keys(shortcut).forEach(key => {
            if (key === 'callback' || key === 'id') return;
            if (shortcut[key] === false) {
              callme = false;
              keysTmpArray.splice(0, keysTmpArray.length);
              return;
            }
            keysTmpArray.push(key);
          });
          if (callme) {
            shortcut.callback(keysTmpArray);
          }
        }
      });
    } else if (event.type === 'keyup') {
      this.shortcuts.forEach(shortcut => {
        if (shortcut[event.keycode] !== undefined) shortcut[event.keycode] = false;
      });
    }
  }
}

const iohook = new IOHook();

module.exports = iohook;
