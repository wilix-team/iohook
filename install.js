'use strict';

const proc = require('child_process');
const path = require('path');
const fs = require('fs');
const os = require('os');

if (process.argv[2] == 'install') {
  try {
    require('./index.js');
  } catch (e) {
    // Need compile
    console.log('Recompiling iohook for your environment');

    let currentPlatform = 'iohook_';
    currentPlatform += os.platform() + '_' + os.arch();
    console.log('Platform is:', currentPlatform);
    
    proc.execSync('git submodule update --init');
    proc.execSync('npm install nan cmake-js');
    proc.execSync('npm run compile');
    if (fs.existsSync(path.join(__dirname, 'build', 'Release', 'iohook.node'))) {
      fs.renameSync(
        path.join(__dirname, 'build', 'Release', 'iohook.node'),
        path.join(__dirname, 'build', 'Release', currentPlatform + '.node')
      );
    }
  }
} else {
  proc.execSync('npm remove nan cmake-js');
  deleteFolderRecursive(path.join(__dirname, 'libuiohook'));
  deleteFolderRecursive(path.join(__dirname, 'build', 'CMakeFiles'));
}

function deleteFolderRecursive(path) {
  if( fs.existsSync(path) ) {
    fs.readdirSync(path).forEach(function(file, index){
      let curPath = path + "/" + file;
      if(fs.lstatSync(curPath).isDirectory()) { // recurse
        deleteFolderRecursive(curPath);
      } else { // delete file
        fs.unlinkSync(curPath);
      }
    });
    fs.rmdirSync(path);
  }
}