'use strict';

const path = require('path');
const fs = require('fs');
const os = require('os');
const nugget = require('nugget');
const rc = require('rc');
const pump = require('pump');
const tfs = require('tar-fs');
const zlib = require('zlib');
const pkg = require('./package.json');
const supportedTargets = require('./package.json').supportedTargets;
const { optionsFromPackage } = require('./helpers');

function onerror(err) {
  throw err;
}

/**
 * Download and Install prebuild
 * @param runtime
 * @param abi
 * @param platform
 * @param arch
 * @param cb Callback
 */
function install(runtime, abi, platform, arch, cb) {
  const essential = runtime + '-v' + abi + '-' + platform + '-' + arch;
  const pkgVersion = pkg.version;
  const currentPlatform = 'iohook-v' + pkgVersion + '-' + essential;

  console.log('Downloading prebuild for platform:', currentPlatform);
  let downloadUrl =
    'https://github.com/wilix-team/iohook/releases/download/v' +
    pkgVersion +
    '/' +
    currentPlatform +
    '.tar.gz';

  let nuggetOpts = {
    dir: os.tmpdir(),
    target: 'prebuild.tar.gz',
    strictSSL: true,
  };

  let npmrc = {};

  try {
    rc('npm', npmrc);
  } catch (error) {
    console.warn('Error reading npm configuration: ' + error.message);
  }

  if (npmrc && npmrc.proxy) {
    nuggetOpts.proxy = npmrc.proxy;
  }

  if (npmrc && npmrc['https-proxy']) {
    nuggetOpts.proxy = npmrc['https-proxy'];
  }

  if (npmrc && npmrc['strict-ssl'] === false) {
    nuggetOpts.strictSSL = false;
  }

  nugget(downloadUrl, nuggetOpts, function (errors) {
    if (errors) {
      const error = errors[0];

      if (error.message.indexOf('404') === -1) {
        onerror(error);
      } else {
        console.error(
          'Prebuild for current platform (' + currentPlatform + ') not found!'
        );
        console.error('Try to build for your platform manually:');
        console.error('# cd node_modules/iohook;');
        console.error('# npm run build');
        console.error('');
      }
    }

    let options = {
      readable: true,
      writable: true,
      hardlinkAsFilesFallback: true,
    };

    let binaryName;
    let updateName = function (entry) {
      if (/\.node$/i.test(entry.name)) binaryName = entry.name;
    };
    let targetFile = path.join(__dirname, 'builds', essential);
    let extract = tfs.extract(targetFile, options).on('entry', updateName);
    pump(
      fs.createReadStream(path.join(nuggetOpts.dir, nuggetOpts.target)),
      zlib.createGunzip(),
      extract,
      function (err) {
        if (err) {
          return onerror(err);
        }
        cb();
      }
    );
  });
}

const options = optionsFromPackage();

if (process.env.npm_config_targets) {
  options.targets = options.targets.concat(
    process.env.npm_config_targets.split(',')
  );
}
if (process.env.npm_config_targets === 'all') {
  options.targets = supportedTargets.map((arr) => [arr[0], arr[2]]);
  options.platforms = ['win32', 'darwin', 'linux'];
  options.arches = ['x64', 'ia32'];
}
if (process.env.npm_config_platforms) {
  options.platforms = options.platforms.concat(
    process.env.npm_config_platforms.split(',')
  );
}
if (process.env.npm_config_arches) {
  options.arches = options.arches.concat(
    process.env.npm_config_arches.split(',')
  );
}

// Choice prebuilds for install
if (options.targets.length > 0) {
  let chain = Promise.resolve();
  options.targets.forEach(function (target) {
    if (typeof target === 'object') {
      chain = chain.then(function () {
        return new Promise(function (resolve) {
          console.log(target.runtime, target.abi, target.platform, target.arch);
          install(
            target.runtime,
            target.abi,
            target.platform,
            target.arch,
            resolve
          );
        });
      });
      return;
    }
    let parts = target.split('-');
    let runtime = parts[0];
    let abi = parts[1];
    options.platforms.forEach(function (platform) {
      options.arches.forEach(function (arch) {
        if (platform === 'darwin' && arch === 'ia32') {
          return;
        }
        chain = chain.then(function () {
          return new Promise(function (resolve) {
            console.log(runtime, abi, platform, arch);
            install(runtime, abi, platform, arch, resolve);
          });
        });
      });
    });
  });
} else {
  const runtime = process.versions['electron'] ? 'electron' : 'node';
  const abi = process.versions.modules;
  const platform = process.platform;
  const arch = process.arch;
  install(runtime, abi, platform, arch, function () {});
}
