// Modules to control application life and create native browser window
const { app, BrowserWindow, Menu } = require('electron');
app.allowRendererProcessReuse = false;
const path = require('path');

function createWindow() {
  //remove application menu on Windows
  //if (process.platform == "win32") Menu.setApplicationMenu(null);

  // Create the browser window.
  const mainWindow = new BrowserWindow({
    width: 1000,
    height: 600,
    webPreferences: {
      nodeIntegration: true,
    },
  });

  // and load the index.html of the app.
  mainWindow.loadFile('index.html');

  // Open the DevTools.
  mainWindow.webContents.openDevTools();
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.whenReady().then(() => {
  createWindow();
});

// Quit when all windows are closed
app.on('window-all-closed', function () {
  app.quit();
});
