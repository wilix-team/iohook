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
				"-framework ApplicationServices"
			]
		},
		"include_dirs": [
			'node_modules/nan',
			'libuiohook/include',
			'libuiohook/src'
		]
	}]
}
