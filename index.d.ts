import { EventEmitter } from "events"

declare module "iohook" {
  export default class IOHook extends EventEmitter {}
}
