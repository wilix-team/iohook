{
	"variables": {
    	"openssl_fips" : "" 
	},
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
        'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
		"include_dirs": [
			"<!@(node -p \"require('node-addon-api').include\")",
			"libuiohook/include",
		],
		"configurations": {
			"Release": {
				"msvs_settings": {
					"VCCLCompilerTool": {
						'ExceptionHandling': 1
					}
				}
			}
		}
	}]
}