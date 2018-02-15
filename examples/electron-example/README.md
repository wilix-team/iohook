### iohook Electron example
This is simple example, how to connect iohook to electron app.

### IMPORTANT
iohook is native lib and require compilation in system before use,
as known Electron have own environment and require electron-rebuild for recompile native libs.
But iohook based on cmake and require following lines in your **package.json** file:

```json
"cmake-js": {
  "runtime": "electron",
  "runtimeVersion": "1.4.7"
}
```
**NOTE: Please don't forget type your version of electron**