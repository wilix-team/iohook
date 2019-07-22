const iohook = require('../index.js');

iohook.on("mouseup", (event) => console.log(event));

let clickpropagation = true;

iohook.on("keyup", (event) => {
  if (event.keycode !== 57) return; // space key

  clickpropagation ? iohook.disableClickPropagation(): iohook.enableClickPropagation();
  clickpropagation = !clickpropagation;
});

iohook.start(true);

console.log('Hook started. Use space key to enable or disable click propagation');
