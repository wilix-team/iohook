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

  let reqOpts = {url: downloadUrl};
  let tempFile = path.join(os.tmpdir(), 'prebuild.tar.gz');
  let req = get(reqOpts, function(err, res) {
    if (err) {
      return onerror(err);
    }
    if (res.statusCode !== 200) {
      if (res.statusCode === 404) {
        console.error('Prebuild for current platform (' + currentPlatform + ') not found!');
        console.error('Try to compile for your platform:');
        console.error('# cd node_modules/iohook;');
        console.error('# npm run compile');
        console.error('');
        return onerror('Prebuild for current platform (' + currentPlatform + ') not found!');
      }
      return onerror('Bad response from prebuild server. Code: ' + res.statusCode);
    }
    pump(res, fs.createWriteStream(tempFile), function(err) {
      if (err) {
        throw err;
      }
      let options = {
        readable: true,
        writable: true,
        hardlinkAsFilesFallback: true
      };
      let binaryName;
      let updateName = function(entry) {
        if (/\.node$/i.test(entry.name)) binaryName = entry.name
      };
      let targetFile = path.join(__dirname, 'builds', essential);
      let extract = tfs.extract(targetFile, options)
        .on('entry', updateName);
      pump(fs.createReadStream(tempFile), zlib.createGunzip(), extract, function(err) {
        if (err) {
          return onerror(err);
        }
        cb()
      })
    })
  });

  req.setTimeout(30 * 1000, function() {
    req.abort()
  })
}

/**
 * Return options for iohook from package.json
 * @return {Object}
 */
function optionsFromPackage(attempts) {
  attempts = attempts || 2;
  if (attempts > 5) {
    console.log('Can\'t resolve main package.json file');
    return {
      targets: [],
      platforms: [process.platform],
      arches: [process.arch]
    }
  }
  let mainPath = Array(attempts).join("../");
  try {
    const content = fs.readFileSync(path.join(__dirname, mainPath, 'package.json'), 'utf-8');
    const packageJson = JSON.parse(content);
    const opts = packageJson.iohook || {};
    if (!opts.targets) {
      opts.targets = []
    }
    if (!opts.platforms) opts.platforms = [process.platform];
    if (!opts.arches) opts.arches = [process.arch];
    return opts
  } catch (e) {
    return optionsFromPackage(attempts + 1);
  }
}

const options = optionsFromPackage();
if (process.env.npm_config_targets) {
  options.targets = options.targets.concat(process.env.npm_config_targets.split(','));
}
options.targets = options.targets.map(targetStr => targetStr.split('-'));
if (process.env.npm_config_targets === 'all') {
  options.targets = support.targets;
  options.platforms = ['win32', 'darwin', 'linux'];
  options.arches = ['x64', 'ia32']
}
if (process.env.npm_config_platforms) {
  options.platforms = options.platforms.concat(process.env.npm_config_platforms.split(','));
}
if (process.env.npm_config_arches) {
  options.arches = options.arches.concat(process.env.npm_config_arches.split(','));
}

// Choice prebuilds for install
if (options.targets.length > 0) {
  let chain = Promise.resolve();
  options.targets.forEach(function(parts) {
    let runtime = parts[0];
    let abi = parts[1];
    options.platforms.forEach(function(platform) {
      options.arches.forEach(function(arch) {
        if (platform === 'darwin' && arch === 'ia32') {
          return;
        }
        chain = chain.then(function() {
          return new Promise(function(resolve) {
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
  install(runtime, abi, platform, arch, function() {
  })
}
