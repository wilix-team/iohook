# OS Support

Every iohook version is built on Linux, macOS and Windows. It has been tested on:

- Ubuntu 20.04
- macOS Big Sur 11.0.1 (Intel only for now) and older
- Windows 10 x64/x32. Requires `Visual C++ Runtime Redistributable` installed on machine either in development or production. Make sure to inform users.

::: tip INFO
Recent versions of NodeJS are not available for 32bit architecture on Linux after Node 13, so we decided to drop support for 32bit Linux prebuilt after v0.7.3.
:::
