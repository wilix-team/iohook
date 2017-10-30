import { EventEmitter } from 'events'

declare class IOHook extends EventEmitter {
  start(enableLogger: boolean): void
  stop(): void
  load(): void
  unload(): void
  setDebug(mode: boolean): void
}

declare const iohook: IOHook

export = iohook
