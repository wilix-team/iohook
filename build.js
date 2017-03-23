var prebuild = require('prebuild/prebuild')
var upload = require('prebuild/upload')
var spawn = require('child_process').spawn
var async = require('async')
var extend = require('xtend')
var path = require('path')
var abi = require('node-abi')

var cmakeJsPath = path.join(__dirname, 'node_modules', '.bin', process.platform === 'darwin' ? 'cmake-js' : 'cmake-js.cmd')

var allTargets = abi.supportedTargets.concat(abi.deprecatedTargets.map(function (dt) {
  dt.runtime = dt.runtime === 'node' ? 'iojs' : 'electron'
  return dt
}))
// console.log(allTargets)

var run = function (args, cb) {
  var extraArgs = []
  args = args.map(function (arg) {
    if (arg.indexOf('--target=') > -1) {
      allTargets.forEach(function (target) {
        if (target.target === arg.split('=')[1]) {
          if (args.indexOf('--runtime=' + target.runtime) === -1) {
            extraArgs.push('--runtime=' + target.runtime)
          }
        }
      })
      return arg.replace('--target=', '--runtime-version=')
    }
    return arg
  }).concat(extraArgs)

  console.log(cmakeJsPath + ' ' + args.join(' '))

  var proc = spawn(cmakeJsPath, args, {
    env: process.env
  })
  proc.stdout.pipe(process.stdout)
  proc.stderr.pipe(process.stderr)
  proc.on('exit', function (code, sig) {
    if (code === 1) {
      return cb(new Error('Failed to build...'))
    }
    cb()
  })
}

var cmakeWrap = {
  todo: [],
  parseArgv: function (argv) {
    this.todo.push({
      name: argv[2],
      args: argv.slice(2)
    })
  },
  commands: {
    build: run,
    clean: run,
    configure: run,
    rebuild: run,
    update: run,
    install: run,
    list: run
  }
}

var opts = {
  upload: process.env.GITHUB_ACCESS_TOKEN,
  target: process.versions.node,
  runtime: 'node',
  arch: process.env.ARCH ? process.env.ARCH.replace('i686', 'ia32').replace('x86_64', 'x64') : process.arch,
  libc: process.env.LIBC,
  platform: process.platform,
  all: false,
  force: false,
  debug: false,
  verbose: false,
  path: '.',
  log: {
    verbose: console.log.bind(console),
    error: console.log.bind(console)
  },
  pkg: require('./package.json'),
  gyp: cmakeWrap,
  prebuild: allTargets.filter(function (target) {
    return target.target !== '0.10.48'
      && target.target !== '0.2.0'
      && target.target !== '0.9.1'
      && target.target !== '0.10.0'
      && target.target !== '0.11.0'
      && target.target !== '0.11.10'
      && target.target !== '8.0.0'
      && target.target !== '0.30.0'
      && target.target !== '0.31.0'
      && target.target !== '0.33.0'
  })
}

function compile() {
  var files = []
  async.eachSeries(opts.prebuild, function (target, next) {
    console.log(target)
    prebuild(opts, target.target, target.runtime, function (err, tarGz) {
      if (err) return next(err)
      files.push(tarGz)
      next()
    })
  }, function (err) {
    if (err) return onbuilderror(err)
    uploadFiles(files)
  })

  function uploadFiles (files) {
    console.log('Uploading ' + files.length + ' prebuilds(s) to Github releases')
    upload(extend(opts, {files: files}), function (err, result) {
      if (err) return onbuilderror(err)
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
    })
  }
}

function onbuilderror (err) {
  if (!err) return
  console.error(err)
  process.exit(2)
}

compile()
