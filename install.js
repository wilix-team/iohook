'use strict';

const proc = require('child_process');
const path = require('path');
const fs = require('fs');
if (process.argv[2] == 'install') {
  try {
    require('./index.js');
  } catch (e) {
    // Need compile
    console.log('Recompiling iohook for your environment');
    proc.execSync('git submodule update --init');
    proc.execSync('npm install nan cmake-js');
    proc.execSync('npm run compile');
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