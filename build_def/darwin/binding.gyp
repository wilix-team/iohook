{
	"targets": [{
		"target_name": "iohook",
		"win_delay_load_hook": "true",
		"type": "loadable_module",
		"sources": [
			"src/iohook.cc",
			"src/iohook.h"
		],
		"dependencies": [
			"./uiohook.gyp:uiohook"
		],
		"cflags": [
			"-std=c99"
		],
		"link_settings": {
				"libraries": [
						"-Wl,-rpath,<!(node -e \"console.log('builds/' + (process.versions['electron'] ? 'electron' : 'node') + '-v' + process.versions.modules + '-' + process.platform + '-' + process.arch + '/build/Release')\")",
						"-Wl,-rpath,<!(pwd)/build/Release/"
				]
		},
		"include_dirs": [
			"<!(node -e \"require('nan')\")",
			"libuiohook/include"
		],
		"configurations": {
			"Release": {
			}
		}
	}]
}
