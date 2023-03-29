{
	"variables": {
    	"openssl_fips" : "" 
	},
	"targets": [{
		"target_name": "uiohook",
		"type": "shared_library",
		"sources": [
			"libuiohook/include/uiohook.h",
			"libuiohook/src/logger.c",
			"libuiohook/src/logger.h",
			"libuiohook/src/windows/input_helper.h",
			"libuiohook/src/windows/input_helper.c",
			"libuiohook/src/windows/input_hook.c",
			"libuiohook/src/windows/post_event.c",
			"libuiohook/src/windows/system_properties.c"
		],
		"include_dirs": [
			"<!(node -e \"require('node-addon-api').include\")",
			"libuiohook/include",
			"libuiohook/src"
		],
		"defines": [
			"NAPI_VERSION=7"
		]
	}]
}
