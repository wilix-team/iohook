const ioHook = require('../../index');
const robot = require('robotjs');

describe('Keyboard events', () => {
  afterEach(() => {
    ioHook.stop();
  });

  it('receives the text "hello world" on keyup event', (done) => {
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
      expect(event.keycode).toEqual(chars[i].keycode);
      expect(event.type).toEqual('keydown');
    });
    ioHook.on('keyup', (event) => {
      expect(event.keycode).toEqual(chars[i].keycode);
      expect(event.type).toEqual('keyup');

      if (i === chars.length - 1) {
        done();
      }

      i += 1;
    });
    ioHook.start();

    for (const char of chars) {
      robot.keyTap(char.value);
    }
  });
});
