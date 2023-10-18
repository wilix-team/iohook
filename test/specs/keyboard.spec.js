const ioHook = require('../../index');

describe('Keyboard events', () => {
  test('test', async () => {
    ioHook.on('keydown', (event) => {
      console.log(event);
    });
    ioHook.load();
    ioHook.start();

    // ioHook.setDebug(true);

    await new Promise((reslove) => setTimeout(() => reslove(), 1000));
    console.log(1000);
    ioHook.stop();
    ioHook.unload();

    await new Promise((reslove) => setTimeout(() => reslove(), 1000));
    console.log(1000);
    ioHook.load();
    ioHook.start();

    await new Promise((reslove) => setTimeout(() => reslove(), 1000));
    console.log(1000);
    ioHook.stop();
    ioHook.unload();
    expect(true).toEqual(true);
  });
});
