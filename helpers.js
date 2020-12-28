const path = require('path');
const fs = require('fs')

/**
 * Return options for iohook from package.json
 * @return {Object}
 */
function optionsFromPackage(attempts) {
  attempts = attempts || 2;
  if (attempts > 5) {
    console.log("Can't resolve main package.json file");
    return {
      targets: [],
      platforms: [process.platform],
      arches: [process.arch],
    };
  }
  let mainPath = Array(attempts).join("../");
  try {
    const content = fs.readFileSync(
      path.join(__dirname, mainPath, "package.json"),
      "utf-8"
    );
    const packageJson = JSON.parse(content);
    const opts = packageJson.iohook || {};
    if (!opts.targets) {
      opts.targets = [];
    }
    if (!opts.platforms) opts.platforms = [process.platform];
    if (!opts.arches) opts.arches = [process.arch];
    return opts;
  } catch (e) {
    return optionsFromPackage(attempts + 1);
  }
}

module.exports = { optionsFromPackage };
