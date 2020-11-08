{
	"targets": [{
		"target_name": "uiohook",
		"type": "shared_library",
		"sources": [
			"libuiohook/include/uiohook.h",
			"libuiohook/src/logger.c",
			"libuiohook/src/logger.h",
			"libuiohook/src/darwin/input_helper.h",
			"libuiohook/src/darwin/input_helper.c",
			"libuiohook/src/darwin/input_hook.c",
			"libuiohook/src/darwin/post_event.c",
			"libuiohook/src/darwin/system_properties.c"
		],
		"cflags": [
			"-std=c99"
		],
		"defines": [
			'USE_IOKIT=1',
			'USE_OBJC=1'
		],
		"link_settings": {
			"libraries": [
				"-framework IOKit",
				"-framework Carbon",
				"-framework ApplicationServices",
				"-lobjc",
				"-Wl,-rpath,<!(node -e \"console.log('builds/' + process.env.gyp_iohook_runtime + '-v' + process.env.gyp_iohook_abi + '-' + process.env.gyp_iohook_platform + '-' + process.env.gyp_iohook_arch + '/build/Release')\")",
				"-Wl,-rpath,<!(pwd)/build/Release/"
			]
		},
		"include_dirs": [
			"<!(node -e \"require('nan')\")",
			'libuiohook/include',
			'libuiohook/src'
		]
	}]
}
