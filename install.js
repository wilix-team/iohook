'use strict';

const path = require('path');
const fs = require('fs');
const os = require('os');
const get = require('simple-get');
const pump = require('pump');
const tfs = require('tar-fs');
const zlib = require('zlib');
const pkg = require('./package.json');
const support = require('./support');

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
  const currentPlatform = pkg.name + '-v' + pkgVersion + '-' + essential;
  
  console.log('Downloading prebuild for platform:', currentPlatform);
  let downloadUrl = 'https://github.com/WilixLead/iohook/releases/download/v' + pkgVersion + '/' + currentPlatform + '.tar.gz';
  
  let reqOpts = { url: downloadUrl };
  let tempFile = path.join(os.tmpdir(), 'prebuild.tar.gz');
  let req = get(reqOpts, function (err, res) {
    if (err) {
      return onerror(err);
    }
    if (res.statusCode !== 200) {
      if (res.statusCode === 400) {
        console.error('Prebuild for current platform (' + currentPlatform + ') not found!');
        console.error('Try to compile for your platform:');
        console.error('# cd node_modules/iohook;');
        console.error('# npm run compile');
        console.error('');
        return onerror('Prebuild for current platform (' + currentPlatform + ') not found!');
      }
      return onerror('Bad response from prebuild server. Code: ' + res.statusCode);
    }
    pump(res, fs.createWriteStream(tempFile), function (err) {
      if (err) {
        throw err;
      }
      let options = {
        readable: true,
        writable: true,
        hardlinkAsFilesFallback: true
      };
      let binaryName;
      let updateName = function (entry) {
        if (/\.node$/i.test(entry.name)) binaryName = entry.name
      };
      let targetFile = path.join(__dirname, 'builds', essential);
      let extract = tfs.extract(targetFile, options)
        .on('entry', updateName);
      pump(fs.createReadStream(tempFile), zlib.createGunzip(), extract, function (err) {
        if (err) {
          return onerror(err);
        }
        cb()
      })
    })
  });

  req.setTimeout(30 * 1000, function () {
    req.abort()
  })
}

/**
 * Return options for iohook from package.json
 * @return {Object}
 */
function optionsFromPackage(attempts) {
  attempts = attempts || 1;
  if (attempts > 5) {
    throw new Error('Can\'t resolve main package.json file');
  }
  let mainPath = attempts === 1 ? './' : Array(attempts).join("../");
  console.log(mainPath);
  try {
    const content = fs.readFileSync(path.join(mainPath, 'package.json'), 'utf-8');
    const packageJson = JSON.parse(content);

    console.log(packageJson);
    return packageJson.iohook || {
        targets: [],
        platforms: [],
        arches: []
      };
  } catch (e) {
    return optionsFromPackage(attempts + 1);
  }
}

const options = optionsFromPackage();
if (process.env.npm_config_targets) {
  options.targets = options.targets.concat(process.env.npm_config_targets.split(','));
}
if (process.env.npm_config_platforms) {
  options.platforms = options.platforms.concat(process.env.npm_config_platforms.split(','));
}
if (process.env.npm_config_arches) {
  options.arches = options.arches.concat(process.env.npm_config_arches.split(','));
}

// Choice prebuilds for install 
if (process.argv.indexOf('--all') > -1 || options.targets) {
  let chain = Promise.resolve();
  let targets;
  if (process.env.npm_config_targets === 'all') { // If need install all targets
    targets = support.targets
  } else {
    targets = options.targets.map(targetStr => targetStr.split('-'));
  }
  let platforms = options.platforms ? options.platforms : ['win32', 'darwin', 'linux'];
  let arches = options.arches ? options.arches : ['x64', 'ia32'];
  
  targets.forEach(function (parts) {
    let runtime = parts[0];
    let version = parts[1];
    platforms.forEach(function (platform) {
      arches.forEach(function (arch) {
        if (platform === 'darwin' && arch === 'ia32') {
          return;
        }
        chain = chain.then(function () {
          return new Promise(function (resolve) {
            let abi = support.abis[runtime][version];
            console.log(runtime, abi, platform, arch);
            install(runtime, abi, platform, arch, resolve)
          })
        })
      })
    })
  })
} else {
  const runtime = process.versions['electron'] ? 'electron' : 'node';
  const abi = process.versions.modules;
  const platform = process.platform;
  const arch = process.arch;
  install(runtime, abi, platform, arch, function () {})
}
