{
  "variables": {
    'v8_enable_pointer_compression': 1
  },
	"targets": [{
		"target_name": "uiohook",
		"type": "shared_library",
		"sources": [
			"libuiohook/include/uiohook.h",
			"libuiohook/src/logger.c",
			"libuiohook/src/logger.h",
			"libuiohook/src/x11/input_helper.h",
			"libuiohook/src/x11/input_helper.c",
			"libuiohook/src/x11/input_hook.c",
			"libuiohook/src/x11/post_event.c",
			"libuiohook/src/x11/system_properties.c"
		],
		"include_dirs": [
			'node_modules/nan',
			'libuiohook/include',
			'libuiohook/src'
		]
	}]
}
