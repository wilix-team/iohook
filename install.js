'use strict';

const path = require('path');
const fs = require('fs');
const os = require('os');
var get = require('simple-get')
var pump = require('pump')
var tfs = require('tar-fs')
var zlib = require('zlib')
var pkg = require('./package.json')
var support = require('./support')

function onerror(err) {
  console.error(err)
}

function install(runtime, abi, platform, arch, cb) {
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
        var targetFile = path.join(__dirname, 'builds', essential)
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
  let targets
  if (process.env.npm_config_targets === 'all') {
    targets = support.targets
  } else {
    targets = process.env.npm_config_targets
      .split(',')
      .map(function (targetStr) {
        return targetStr.split('-')
      })
  }
  var platforms = process.env.npm_config_platforms ? process.env.npm_config_platforms.split(',') : ['win32', 'darwin']
  var arches = process.env.npm_config_arches ? process.env.npm_config_arches.split(',') : ['x64', 'ia32']
  targets.forEach(function (parts) {
    var runtime = parts[0]
    var version = parts[1]
    platforms.forEach(function (platform) {
      arches.forEach(function (arch) {
        if (platform === 'darwin' && arch === 'ia32') return
        chain = chain.then(function () {
          return new Promise(function (resolve) {
            var abi = support.abis[runtime][version]
            console.log(runtime, abi, platform, arch)
            install(runtime, abi, platform, arch, resolve)
          })
        })
      })
    })
  })
} else {
  const runtime = process.versions['electron'] ? 'electron' : 'node'
  const abi = process.versions.modules
  const platform = process.platform
  const arch = process.arch
  install(runtime, abi, platform, arch, function () {})
}
