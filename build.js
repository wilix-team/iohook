const spawn = require('child_process').spawn;
const fs = require('fs-extra');
const path = require('path');
const argv = require('minimist')(process.argv.slice(2), {});

initBuild();
function initBuild() {
  Promise.resolve()
    .then(() => copyGyps())
    .then(() => build())
    .then(() => moveDist())
    .then(() => unlinkGyps())
    .catch((err) => {
      console.error(err);
      process.exit(1);
    });
}

function copyGyps() {
  switch (process.platform) {
    case 'win32':
    case 'darwin':
      fs.copySync(
        path.join(__dirname, 'gyp', process.platform, 'binding.gyp'),
        path.join(__dirname, 'binding.gyp')
      );
      fs.copySync(
        path.join(__dirname, 'gyp', process.platform, 'uiohook.gyp'),
        path.join(__dirname, 'uiohook.gyp')
      );
      break;
    default:
      fs.copySync(
        path.join(__dirname, 'gyp', 'linux', 'binding.gyp'),
        path.join(__dirname, 'binding.gyp')
      );
      fs.copySync(
        path.join(__dirname, 'gyp', 'linux', 'uiohook.gyp'),
        path.join(__dirname, 'uiohook.gyp')
      );
      break;
  }
}

function unlinkGyps() {
  try {
    fs.unlinkSync(path.join(__dirname, 'binding.gyp'));
    fs.unlinkSync(path.join(__dirname, 'uiohook.gyp'));
  } catch (e) {}
}

function build() {
  const arch = process.env.ARCH
    ? process.env.ARCH.replace('i686', 'ia32').replace('x86_64', 'x64')
    : process.arch;

  const gypJsPath = path.join(
    __dirname,
    'node_modules',
    '.bin',
    process.platform === 'win32' ? 'node-gyp.cmd' : 'node-gyp'
  );

  return new Promise(function (resolve, reject) {
    let args = ['configure', 'rebuild', '--arch=' + arch];

    if (arch === 'x64') {
      args.push('--v8_enable_pointer_compression=1');
    } else {
      args.push('--v8_enable_pointer_compression=0');
      args.push('--v8_enable_31bit_smis_on_64bit_arch=1');
    }

    if (process.platform !== 'win32') {
      args.push('--build_v8_with_gn=false');
      args.push('--enable_lto=false');
    }

    if (process.platform === 'win32') {
      process.env.msvs_toolset = 15;
      process.env.msvs_version = argv.msvs_version || 2017;

      args.push('--msvs_version=' + process.env.msvs_version);
    } else {
      process.env.gyp_iohook_platform = process.platform;
      process.env.gyp_iohook_arch = arch;
    }

    let proc = spawn(gypJsPath, args, {
      env: process.env,
    });
    proc.stdout.pipe(process.stdout);
    proc.stderr.pipe(process.stderr);
    proc.on('exit', function (code, sig) {
      if (code === 1) {
        return reject(new Error('Failed to build...'));
      }
      resolve();
    });
  });
}

function moveDist() {
  const srcFiles = [];
  const dstFiles = [];
  srcFiles.push(path.join('build', 'Release', 'iohook.node'));
  dstFiles.push(path.join('dist', 'iohook.node'));

  const FILES_TO_ARCHIVE = {
    win32: ['uiohook.dll'],
    linux: ['uiohook.so'],
    darwin: ['uiohook.dylib'],
  };
  FILES_TO_ARCHIVE[process.platform].forEach((file) => {
    srcFiles.push(path.join('build', 'Release', file));
    dstFiles.push(path.join('dist', file));
  });

  srcFiles.forEach((_, i) => {
    fs.moveSync(srcFiles[i], dstFiles[i], { overwrite: true });
  });

  return dstFiles;
}
