import { EventEmitter } from 'events'

/**
 * Native module for hooking keyboard and mouse events
 */
declare class IOHook extends EventEmitter<IOHookEvent> {
  /**
   * Start hooking engine. Call it when you ready to receive events
   * @param {boolean} enableLogger If true, module will publish debug information to stdout
   */
  start(enableLogger: boolean): void

  /**
   * Stop rising keyboard/mouse events
   */
  stop(): void

  /**
   * Manual native code load. Call this function only if unload called before
   */
  load(): void

  /**
   * Unload native code and free memory and system hooks
   */
  unload(): void

  /**
   * Enable/Disable stdout debug
   * @param {boolean} mode
   */
  setDebug(mode: boolean): void

  /**
   * Register global shortcut. When all keys in keys array pressed, callback will be called
   * @param {Array<string|number>} keys Array of keycodes
   * @param {Function} callback Callback for call when shortcut pressed
   * @return {number} ShortcutId for unregister
   */
  registerShortcut(keys, callback): number

  /**
   * Unregister shortcut by ShortcutId
   * @param shortcutId
   */
  unregisterShortcut(shortcutId): void

  /**
   * Unregister all shortcuts
   */
  unregisterAllShortcuts(): void
}

declare interface IOHookEvent {
  type: string
  keychar?: number
  keycode?: number 
  rawcode?: number
  button?: number
  clicks?: number
  x?: number
  y?: number
}

declare const iohook: IOHook;

export = iohook
