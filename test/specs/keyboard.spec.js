const ioHook = require('../../index');
const robot = require('robotjs');

describe('Keyboard events', () => {
    it('receives the text "Hello World"', (done) => {
        let currentKey = 0;

        const actualCharacters = [];
        const expectedCharacters = ['H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'];

        ioHook.on('keydown', (event) => {
            actualCharacters.push(String.fromCharCode(event.keychar));

            if (currentKey === expectedCharacters.length) {
                expect(actualCharacters).toEqual(expectedCharacters);
            }

            currentKey += 1;
            done();
        });
        ioHook.start();

        robot.typeString('Hello World');
    });
});
