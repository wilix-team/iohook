'use strict';

const path = require('path');
const fs = require('fs');
const os = require('os');
var get = require('simple-get')
var pump = require('pump')
var tfs = require('tar-fs')
var zlib = require('zlib')
var pkg = require('./package.json')

function onerror(err) {
  console.error(err)
}

function install(runtime, abi, platform, arch, cb) {
  // example prebuild files:
  // - iohook-prebuild-test-v0.1.4-electron-v47-darwin-x64.tar.gz
  // - iohook-prebuild-test-v0.1.4-node-v51-darwin-x64.tar.gz
  const essential = runtime + '-v' + abi + '-' + platform + '-' + arch
  const pkgVersion = pkg.version
  const currentPlatform = pkg.name + '-v' + pkgVersion + '-' + essential
  console.log('Platform is:', currentPlatform);
  const downloadUrl = 'https://github.com/vespakoen/iohook/releases/download/v' + pkgVersion + '/' + currentPlatform + '.tar.gz'
  var reqOpts = { url: downloadUrl }
  var tempFile = path.join(os.tmpdir(), 'prebuild.tar.gz')
  var req = get(reqOpts, function (err, res) {
    if (err) return onerror(err)
    console.log(res.statusCode, downloadUrl)
    if (res.statusCode !== 200) return onerror()
      console.log('downloading to @', tempFile)
      pump(res, fs.createWriteStream(tempFile), function (err) {
        var options = {
          readable: true,
          writable: true,
          hardlinkAsFilesFallback: true
        }
        var binaryName
        var updateName = function (entry) {
          if (/\.node$/i.test(entry.name)) binaryName = entry.name
        }
        var targetFile = path.join(__dirname, 'lib', essential)
        var extract = tfs.extract(targetFile, options)
          .on('entry', updateName)

        pump(fs.createReadStream(tempFile), zlib.createGunzip(), extract, function (err) {
          if (err) return onerror(err)
          cb()
        })
    })
  })

  req.setTimeout(30 * 1000, function () {
    req.abort()
  })
}

try {
  require('./index.js');
} catch (e) {
  if (process.argv.indexOf('--all') > -1 || process.env.IOHOOK_INSTALL_ALL) {
    const targets = require('node-abi').supportedTargets.filter(function (target) { return target.target !== '0.10.48' })
    let chain = Promise.resolve()
    targets.forEach(function (target) {
      console.log(target);
      ['win32', 'darwin'].forEach(function (platform) {
        ['x64', 'ia32'].forEach(function (arch) {
          if (platform === 'darwin' && arch === 'ia32') return
          chain = chain.then(function () {
            return new Promise(function (resolve) {
              install(target.runtime, target.abi, platform, arch, resolve)
            })
          })
        })
      })
    })
  } else {
    const runtime = process.versions['electron'] ? 'electron' : 'node'
    const abi = require('node-abi').getAbi(process.versions[runtime], runtime)
    const platform = os.platform()
    const arch = os.arch()
    install(runtime, abi, platform, arch, function () {})
  }
}
