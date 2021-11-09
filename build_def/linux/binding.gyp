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
			"-std=c++14",
			"-fPIC"
		],
		"defines": [
			"USE_XKBCOMMON"
		],
		"link_settings": {
				"libraries": [
						"-Wl,-rpath,<!(node -e \"console.log('builds/' + process.env.gyp_iohook_runtime + '-v' + process.env.gyp_iohook_abi + '-' + process.env.gyp_iohook_platform + '-' + process.env.gyp_iohook_arch + '/build/Release')\")",
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
