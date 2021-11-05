const EventEmitter = require('events');
const path = require('path');

const runtime = process.versions['electron'] ? 'electron' : 'node';
const essential =
  runtime +
  '-v' +
  process.versions.modules +
  '-' +
  process.platform +
  '-' +
  process.arch;
const modulePath = path.join(
  __dirname,
  'builds',
  essential,
  'build',
  'Release',
  'iohook.node'
);
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
  11: 'mousewheel',
};

class IOHook extends EventEmitter {
  constructor() {
    super();
    this.active = false;
    this.shortcuts = [];
    this.eventProperty = 'keycode';
    this.activatedShortcuts = [];

    this.lastKeydownShift = false;
    this.lastKeydownAlt = false;
    this.lastKeydownCtrl = false;
    this.lastKeydownMeta = false;

    this.load();
    this.setDebug(false);
  }

  /**
   * Start hook process
   * @param {boolean} [enableLogger] Turn on debug logging
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
   * @param {Function} callback Callback for when shortcut pressed
   * @param {Function} [releaseCallback] Callback for when shortcut has been released
   * @return {number} ShortcutId for unregister
   */
  registerShortcut(keys, callback, releaseCallback) {
    let shortcut = {};
    let shortcutId = Date.now() + Math.random();
    keys.forEach((keyCode) => {
      shortcut[keyCode] = false;
    });
    shortcut.id = shortcutId;
    shortcut.callback = callback;
    shortcut.releaseCallback = releaseCallback;
    this.shortcuts.push(shortcut);
    return shortcutId;
  }

  /**
   * Unregister shortcut by ShortcutId
   * @param shortcutId
   */
  unregisterShortcut(shortcutId) {
    this.shortcuts.forEach((shortcut, i) => {
      if (shortcut.id === shortcutId) {
        this.shortcuts.splice(i, 1);
      }
    });
  }

  /**
   * Unregister shortcut via its key codes
   * @param {string} keyCodes Keyboard keys matching the shortcut that should be unregistered
   */
  unregisterShortcutByKeys(keyCodes) {
    // A traditional loop is used in order to access `this` from inside
    for (let i = 0; i < this.shortcuts.length; i++) {
      let shortcut = this.shortcuts[i];

      // Convert any keycode numbers to strings
      keyCodes.forEach((key, index) => {
        if (typeof key !== 'string' && !(key instanceof String)) {
          // Convert to string
          keyCodes[index] = key.toString();
        }
      });

      // Check if this is our shortcut
      Object.keys(shortcut).every((key) => {
        if (key === 'callback' || key === 'id') return;

        // Remove all given keys from keyCodes
        // If any are not in this shortcut, then this shortcut does not match
        // If at the end we have eliminated all codes in keyCodes, then we have succeeded
        let index = keyCodes.indexOf(key);
        if (index === -1) return false; // break

        // Remove this key from the given keyCodes array
        keyCodes.splice(index, 1);
        return true;
      });

      // Is this the shortcut we want to remove?
      if (keyCodes.length === 0) {
        // Unregister this shortcut
        this.shortcuts.splice(i, 1);
        return;
      }
    }
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
   * Specify that key event's `rawcode` property should be used instead of
   * `keycode` when listening for key presses.
   *
   * This allows iohook to be used in conjunction with other programs that may
   * only provide a keycode.
   * @param {Boolean} using
   */
  useRawcode(using) {
    // If true, use rawcode, otherwise use keycode
    this.eventProperty = using ? 'rawcode' : 'keycode';
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
      if (
        (event.type === 'keydown' || event.type === 'keyup') &&
        iohook.shortcuts.length > 0
      ) {
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

    // Keep track of shortcuts that are currently active
    let activatedShortcuts = this.activatedShortcuts;

    if (event.type === 'keydown') {
      this.shortcuts.forEach((shortcut) => {
        if (shortcut[event[this.eventProperty]] !== undefined) {
          // Mark this key as currently being pressed
          shortcut[event[this.eventProperty]] = true;

          let keysTmpArray = [];
          let callme = true;

          // Iterate through each keyboard key in this shortcut
          Object.keys(shortcut).forEach((key) => {
            if (key === 'callback' || key === 'releaseCallback' || key === 'id')
              return;

            // If one of the keys aren't pressed...
            if (shortcut[key] === false) {
              // Don't call the callback and empty our temp tracking array
              callme = false;
              keysTmpArray.splice(0, keysTmpArray.length);

              return;
            }

            // Otherwise, this key is being pressed.
            // Add it to the array of keyboard keys we will send as an argument
            // to our callback
            keysTmpArray.push(key);
          });
          if (callme) {
            shortcut.callback(keysTmpArray);

            // Add this shortcut from our activate shortcuts array if not
            // already activated
            if (activatedShortcuts.indexOf(shortcut) === -1) {
              activatedShortcuts.push(shortcut);
            }
          }
        }
      });
    } else if (event.type === 'keyup') {
      // Mark this key as currently not being pressed in all of our shortcuts
      this.shortcuts.forEach((shortcut) => {
        if (shortcut[event[this.eventProperty]] !== undefined) {
          shortcut[event[this.eventProperty]] = false;
        }
      });

      // Check if any of our currently pressed shortcuts have been released
      // "released" means that all of the keys that the shortcut defines are no
      // longer being pressed
      this.activatedShortcuts.forEach((shortcut) => {
        if (shortcut[event[this.eventProperty]] === undefined) return;

        let shortcutReleased = true;
        let keysTmpArray = [];
        Object.keys(shortcut).forEach((key) => {
          if (key === 'callback' || key === 'releaseCallback' || key === 'id')
            return;
          keysTmpArray.push(key);

          // If any key is true, and thus still pressed, the shortcut is still
          // being held
          if (shortcut[key]) {
            shortcutReleased = false;
          }
        });

        if (shortcutReleased) {
          // Call the released function handler
          if (shortcut.releaseCallback) {
            shortcut.releaseCallback(keysTmpArray);
          }

          // Remove this shortcut from our activate shortcuts array
          const index = this.activatedShortcuts.indexOf(shortcut);
          if (index !== -1) this.activatedShortcuts.splice(index, 1);
        }
      });
    }
  }
}

const iohook = new IOHook();

module.exports = iohook;
