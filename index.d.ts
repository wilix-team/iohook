import { EventEmitter } from 'events';

/**
 * Native module for hooking keyboard and mouse events
 */
declare class IOHook extends EventEmitter {
  /**
   * Start hooking engine. Call it when you ready to receive events
   * @param {boolean} [enableLogger] If true, module will publish debug information to stdout
   */
  start(enableLogger?: boolean): void;

  /**
   * Stop rising keyboard/mouse events
   */
  stop(): void;

  /**
   * Manual native code load. Call this function only if unload called before
   */
  load(): void;

  /**
   * Unload native code and free memory and system hooks
   */
  unload(): void;

  /**
   * Enable/Disable stdout debug
   * @param {boolean} mode
   */
  setDebug(mode: boolean): void;

  /**
   * Specify that key event's `rawcode` property should be used instead of
   * `keycode` when listening for key presses.
   *
   * This allows iohook to be used in conjunction with other programs that may
   * only provide a keycode.
   * @param {Boolean} using
   */
  useRawcode(using: boolean): void;

  /**
   * Enable mouse click propagation (enabled by default).
   * The click event are emitted and propagated.
   */
  enableClickPropagation(): void;

  /**
   * Disable mouse click propagation.
   * The click event are captured and the event emitted but not propagated to the window.
   */
  disableClickPropagation(): void;

  /**
   * Register global shortcut. When all keys in keys array pressed, callback will be called
   * @param {Array<string|number>} keys Array of keycodes
   * @param {Function} callback Callback for when shortcut pressed
   * @param {Function} [releaseCallback] Callback for when shortcut released
   * @return {number} ShortcutId for unregister
   */
  registerShortcut(
    keys: Array<string | number>,
    callback: Function,
    releaseCallback?: Function
  ): number;

  /**
   * Unregister shortcut by ShortcutId
   * @param {number} shortcutId
   */
  unregisterShortcut(shortcutId: number): void;

  /**
   * Unregister shortcut via its key codes
   * @param {Array<string|number>} keys
   */
  unregisterShortcut(keys: Array<string | number>): void;

  /**
   * Unregister all shortcuts
   */
  unregisterAllShortcuts(): void;
}

declare interface IOHookEvent {
  type: string;
  keychar?: number;
  keycode?: number;
  rawcode?: number;
  button?: number;
  clicks?: number;
  x?: number;
  y?: number;
}

declare const iohook: IOHook;

export = iohook;
