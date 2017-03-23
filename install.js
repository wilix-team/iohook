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
  console.log('Downloading prebuild for platform:', currentPlatform);
  const downloadUrl = 'https://github.com/vespakoen/iohook/releases/download/v' + pkgVersion + '/' + currentPlatform + '.tar.gz'
  var reqOpts = { url: downloadUrl }
  var tempFile = path.join(os.tmpdir(), 'prebuild.tar.gz')
  var req = get(reqOpts, function (err, res) {
    if (err) return onerror(err)
    if (res.statusCode !== 200) return onerror()
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

if (process.argv.indexOf('--all') > -1 || process.env.npm_config_targets) {
  let chain = Promise.resolve()
  if (process.argv.indexOf('--all') > -1 || process.env.npm_config_targets === 'all') {
    var targets = require('node-abi')
      .supportedTargets
      .filter(function (target) {
        return target.target !== '0.10.48'
      })
    targets.forEach(function (target) {
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
    var targets = process.env.npm_config_targets.split(',')
    targets.forEach(function (targetStr) {
      var parts = targetStr.split('-')
      chain = chain.then(function () {
        return new Promise(function (resolve) {
          install(parts[0], parts[1], parts[2], parts[3], resolve)
        })
      })
    })
  }
} else {
  const runtime = process.versions['electron'] ? 'electron' : 'node'
  const abi = require('node-abi').getAbi(process.versions[runtime], runtime)
  const platform = os.platform()
  const arch = os.arch()
  install(runtime, abi, platform, arch, function () {})
}
