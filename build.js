var prebuild = require('prebuild/prebuild')
var upload = require('prebuild/upload')
var spawn = require('child_process').spawn
var async = require('async')
var extend = require('xtend')
var path = require('path')

var run = function (args, cb) {
  args = args.map(function (arg) {
    if (arg.indexOf('--target') > -1) {
      return arg.replace('--target', '--runtime-version')
    }
    return arg
  })
  var cmakeJsPath = path.join(__dirname, 'node_modules', 'cmake-js', 'bin', process.platform === 'darwin' ? 'cmake-js' : 'cmake-js.cmd')
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
  target: process.versions.node,
  runtime: 'node',
  arch: process.arch,
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
  prebuild: require('node-abi').supportedTargets,
}
console.log(opts.prebuild)

var files = []
async.eachSeries(opts.prebuild, function (target, next) {
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

function onbuilderror (err) {
  if (!err) return
  console.error(err)
  process.exit(2)
}
