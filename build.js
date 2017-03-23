var upload = require('prebuild/upload')
var spawn = require('child_process').spawn
var async = require('async')
var extend = require('xtend')
var fs = require('fs')
var path = require('path')
var mkdirp = require('mkdirp')
var tar = require('tar-stream')
var zlib = require('zlib')
var pkg = require('./package.json')

function mode(octal) {
  return parseInt(octal, 8)
}

var cmakeJsPath = path.join(
  __dirname,
  'node_modules',
  '.bin',
  process.platform === 'darwin' ? 'cmake-js' : 'cmake-js.cmd'
)

var support = require('./support')
var targets = support.targets
var abis = support.abis
var files = []

var chain = Promise.resolve()
targets.forEach(parts => {
  var runtime = parts[0]
  var version = parts[1]
  chain = chain.then(function () {
    return build(runtime, version)
  })
  .then(function () {
    return tarGz(runtime, version)
  })
})
chain = chain.then(function () {
  return uploadFiles(files)
})

function build(runtime, version) {
  return new Promise(function (resolve, reject) {
    var args = [
      'rebuild',
      '--runtime-version=' + version,
      '--target_arch=' + process.arch,
      '--runtime=' + runtime
    ]
    var proc = spawn(cmakeJsPath, args, {
      env: process.env
    })
    proc.stdout.pipe(process.stdout)
    proc.stderr.pipe(process.stderr)
    proc.on('exit', function (code, sig) {
      if (code === 1) {
        return reject(new Error('Failed to build...'))
      }
      resolve()
    })
  })
}

function tarGz(runtime, version) {
  return new Promise(function (resolve) {
    var filename = 'build/Release/iohook.node'
    var abi = abis[runtime][version]
    var tarPath = 'prebuilds/' + pkg.name + '-v' + pkg.version + '-' + runtime + '-v' + abi + '-' + process.platform + '-' + process.arch + '.tar.gz'
    files.push(tarPath)
    mkdirp(path.dirname(tarPath), function () {
      fs.stat(filename, function (err, st) {
        if (err) return reject(err)
        var tarStream = tar.pack()
        var ws = fs.createWriteStream(tarPath)
        var stream = tarStream.entry({
          name: filename.replace(/\\/g, '/').replace(/:/g, '_'),
          size: st.size,
          mode: st.mode | mode('444') | mode('222'),
          gid: st.gid,
          uid: st.uid
        })
        fs.createReadStream(filename)
          .pipe(stream)
          .on('finish', function () {
            tarStream.finalize()
          })
        tarStream
          .pipe(zlib.createGzip())
          .pipe(ws)
          .on('close', resolve)
      })
    })
  })
}

function uploadFiles (files) {
  return new Promise(function (resolve, reject) {
    console.log('Uploading ' + files.length + ' prebuilds(s) to Github releases')
    var opts = {
      pkg: pkg,
      files: files,
      upload: process.env.GITHUB_ACCESS_TOKEN
    }
    upload(opts, function (err, result) {
      if (err) return reject(err)
      console.log('Found ' + result.old.length + ' prebuild(s) on Github')
      if (result.old.length) {
        result.old.forEach(function (build) {
          console.log('-> ' + build)
        })
      }
      console.log('Uploaded ' + result.new.length + ' new prebuild(s) to Github')
      if (result.new.length) {
        result.new.forEach(function (build) {
          console.log('-> ' + build)
        })
      }
      resolve()
    })
  })
}
