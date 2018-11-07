const ioHook = require('../../index');
const robot = require('robotjs');

describe('Keyboard events', () => {
  afterEach(() => {
    ioHook.stop();
  });

  it('receives the text "hello world" on keyup event', (done) => {
    expect.assertions(22);

    const chars = [
      { keycode: 35, value: 'h' },
      { keycode: 18, value: 'e' },
      { keycode: 38, value: 'l' },
      { keycode: 38, value: 'l' },
      { keycode: 24, value: 'o' },
      { keycode: 57, value: ' ' },
      { keycode: 17, value: 'w' },
      { keycode: 24, value: 'o' },
      { keycode: 19, value: 'r' },
      { keycode: 38, value: 'l' },
      { keycode: 32, value: 'd' }
    ];
    let i = 0;

    ioHook.on('keydown', (event) => {
      expect(event).toEqual({
        keycode: chars[i].keycode,
        type: 'keydown',
        shiftKey: false,
        altKey: false,
        ctrlKey: false,
        metaKey: false
      });
    });
    ioHook.on('keyup', (event) => {
      expect(event).toEqual({
        keycode: chars[i].keycode,
        type: 'keydown',
        shiftKey: false,
        altKey: false,
        ctrlKey: false,
        metaKey: false
      });

      if (i === chars.length - 1) {
        done();
      }

      i += 1;
    });
    ioHook.start();

    setTimeout(() => { // Make sure ioHook starts before anything gets typed
      for (const char of chars) {
        robot.keyTap(char.value);
      }
    }, 50);
  });

  it('recognizes shift key being pressed', (done) => {
    expect.assertions(8);

    ioHook.on('keydown', (event) => {
      expect(event).toEqual({
        type: 'keydown',
        shiftKey: true,
        altKey: false,
        ctrlKey: false,
        metaKey: false
      });
    });
    ioHook.on('keyup', (event) => {
      expect(event).toEqual({
        type: 'keydown',
        shiftKey: true,
        altKey: false,
        ctrlKey: false,
        metaKey: false
      });
    });
    ioHook.start();

    setTimeout(() => { // Make sure ioHook starts before anything gets typed
      robot.keyToggle('shift', 'down');
      robot.keyTap('1');
      robot.keyToggle('shift', 'up');
    }, 50);
  });

  it('recognizes alt key being pressed', (done) => {
    expect.assertions(8);

    ioHook.on('keydown', (event) => {
      expect(event).toEqual({
        type: 'keydown',
        shiftKey: false,
        altKey: true,
        ctrlKey: false,
        metaKey: false
      });
    });
    ioHook.on('keyup', (event) => {
      expect(event).toEqual({
        type: 'keydown',
        shiftKey: false,
        altKey: true,
        ctrlKey: false,
        metaKey: false
      });
    });
    ioHook.start();

    setTimeout(() => { // Make sure ioHook starts before anything gets typed
      robot.keyToggle('alt', 'down');
      robot.keyTap('1');
      robot.keyToggle('alt', 'up');
    }, 50);
  });

  it('recognizes ctrl key being pressed', (done) => {
    expect.assertions(8);

    ioHook.on('keydown', (event) => {
      expect(event).toEqual({
        type: 'keydown',
        shiftKey: false,
        altKey: false,
        ctrlKey: true,
        metaKey: false
      });
    });
    ioHook.on('keyup', (event) => {
      expect(event).toEqual({
        type: 'keydown',
        shiftKey: false,
        altKey: false,
        ctrlKey: true,
        metaKey: false
      });
    });
    ioHook.start();

    setTimeout(() => { // Make sure ioHook starts before anything gets typed
      robot.keyToggle('control', 'down');
      robot.keyTap('1');
      robot.keyToggle('control', 'up');
    }, 50);
  });

  it('recognizes meta key being pressed', (done) => {
    expect.assertions(8);

    ioHook.on('keydown', (event) => {
      expect(event).toEqual({
        type: 'keydown',
        shiftKey: false,
        altKey: false,
        ctrlKey: false,
        metaKey: true
      });
    });
    ioHook.on('keyup', (event) => {
      expect(event).toEqual({
        type: 'keydown',
        shiftKey: false,
        altKey: false,
        ctrlKey: false,
        metaKey: true
      });
    });
    ioHook.start();

    setTimeout(() => { // Make sure ioHook starts before anything gets typed
      robot.keyToggle('command', 'down');
      robot.keyTap('1');
      robot.keyToggle('command', 'up');
    }, 50);
  });

  it('runs a callback when a shortcut has been released', (done) => {
    expect.assertions(2);

    let shortcut = [42, 30];

    ioHook.registerShortcut(shortcut, keys => {
      expect(shortcut.sort()).toEqual(keys.sort());
    }, keys => {
      expect(shortcut.sort()).toEqual(keys.sort());
    });

    setTimeout(() => { // Make sure ioHook starts before anything gets typed
      robot.keyToggle('shift', 'down');
      robot.keyTap('a');
      robot.keyToggle('shift', 'up');
    }, 50);
  });

  it('can unregister a shortcut via its keycodes', (done) => {
    expect.assertions(0);

    let shortcut = [42, 30];

    // Register the shortcut
    ioHook.registerShortcut(shortcut, (event) => {
      // We're unregistering this shortcut. It should not have been called
      expect.not.toHaveBeenCalled();
    });

    // Unregister the shortcut
    ioHook.unregisterShortcutByKeys(shortcut);

    ioHook.start();

    setTimeout(() => { // Make sure ioHook starts before anything gets typed
      robot.keyToggle('shift', 'down');
      robot.keyTap('a');
      robot.keyToggle('shift', 'up');
    }, 50);
  });

  it('can use rawcode instead of keycode when detecting events', (done) => {
    expect.assertions(1);

    let rawCodeShortcut = [65505, 65]; // Shift + A in rawcode

    ioHook.registerShortcut(rawCodeShortcut, (event) => {
      expect.toHaveBeenCalled();
    });

    // Check rawcode detection works
    ioHook.useRawcode(true);
    ioHook.start();

    setTimeout(() => { // Make sure ioHook starts before anything gets typed
      robot.keyToggle('shift', 'down');
      robot.keyTap('a');
      robot.keyToggle('shift', 'up');
    }, 50);
  });
});
