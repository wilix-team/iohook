const spawn = require('child_process').spawn;
const fs = require('fs');
const path = require('path');
const mkdirp = require('mkdirp');
const archiver = require('archiver');
const zlib = require('zlib');
const argv = require('minimist')(
    process.argv.slice(2), {
        // Specify that these arguments should be a string
        "string": ["version", "runtime", "abi"]
    }
);
const pkg = require('./package.json');

function mode(octal) {
  return parseInt(octal, 8)
}

let arch = process.env.ARCH
  ? process.env.ARCH
    .replace('i686', 'ia32')
    .replace('x86_64', 'x64')
  : process.arch;

let cmakeJsPath = path.join(
  __dirname,
  'node_modules',
  '.bin',
  process.platform === 'win32' ? 'cmake-js.cmd' : 'cmake-js'
);

let files = [];
let targets;

// Check if a specific runtime has been specified from the command line
if ("runtime" in argv && "version" in argv && "abi" in argv) {
    targets = [[argv["runtime"],
                argv["version"],
                argv["abi"]]];
} else {
    // If not, use those defined in package.json
    targets = require('./package.json').supportedTargets;
}

let chain = Promise.resolve();

targets.forEach(parts => {
  let runtime = parts[0];
  let version = parts[1];
  let abi = parts[2]
  chain = chain
    .then(function () {
      return build(runtime, version)
    })
    .then(function () {
      return tarGz(runtime, abi)
    })
    .catch(err => {
      console.error(err);
      process.exit(1);
    })
});

chain = chain.then(function () {
  if ("upload" in argv && argv["upload"] == false) {
    // If no upload has been specified, don't attempt to upload
    return;
  }

  return uploadFiles(files)
});

function build(runtime, version) {
  return new Promise(function (resolve, reject) {
    let args = [
      'rebuild',
      '--runtime-version=' + version,
      '--runtime=' + runtime,
      '--arch=' + arch
    ];
    console.log('Compiling iohook for ' + runtime + ' v' + version + '>>>>');
    if (version.split('.')[0] >= 4) {
      process.env.msvs_toolset = 15
      process.env.msvs_version = 2017
    } else {
      process.env.msvs_toolset = 12
      process.env.msvs_version = 2013
    }
    let proc = spawn(cmakeJsPath, args, {
      env: process.env
    });
    proc.stdout.pipe(process.stdout);
    proc.stderr.pipe(process.stderr);
    proc.on('exit', function (code, sig) {
      if (code === 1) {
        return reject(new Error('Failed to build...'))
      }
      resolve()
    })
  })
}

function tarGz(runtime, abi) {
  const filesToArchive = process.platform == 'win32' ? 
    ['build/Release/iohook.node', 'build/Release/uiohook.dll']
  :
    ['build/Release/iohook.node']

  const tarPath = 'prebuilds/' + pkg.name + '-v' + pkg.version + '-' + runtime + '-v' + abi + '-' + process.platform + '-' + arch + '.tar.gz';

  files.push(tarPath)

  mkdirp(path.dirname(tarPath), () => {
    const output = fs.createWriteStream(tarPath);
    const archive = archiver('tar', {
      gzip: true
    });

    archive.pipe(output);

    filesToArchive.forEach(file => {
      archive.append(fs.createReadStream(file), { name: file });
    });

    archive.finalize();
  });
}

function uploadFiles (files) {
  const upload = require('prebuild/upload');
  return new Promise(function (resolve, reject) {
    console.log('Uploading ' + files.length + ' prebuilds(s) to Github releases');
    let opts = {
      pkg: pkg,
      files: files,
      upload: process.env.GITHUB_ACCESS_TOKEN
    };
    upload(opts, function (err, result) {
      if (err) {
        return reject(err);
      }
      console.log('Found ' + result.old.length + ' prebuild(s) on Github');
      if (result.old.length) {
        result.old.forEach(function (build) {
          console.log('-> ' + build)
        })
      }
      console.log('Uploaded ' + result.new.length + ' new prebuild(s) to Github');
      if (result.new.length) {
        result.new.forEach(function (build) {
          console.log('-> ' + build)
        })
      }
      resolve()
    })
  })
}
