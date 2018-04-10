cmake_minimum_required(VERSION 2.8)

project(libuiohook)

# libuihook sources.
if(WIN32 OR WIN64)
	set(UIOHOOK_SRC
		  "${CMAKE_CURRENT_SOURCE_DIR}/include/uiohook.h"
		  "${CMAKE_CURRENT_SOURCE_DIR}/src/logger.c"
		  "${CMAKE_CURRENT_SOURCE_DIR}/src/logger.h"
		  "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/input_helper.h"
		  "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/input_helper.c"
		  "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/input_hook.c"
		  "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/post_event.c"
		  "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/system_properties.c"
		)
elseif(LINUX)
	set(UIOHOOK_SRC
		  "include/uiohook.h"
		  "src/logger.c"
		  "src/logger.h"
		  "src/x11/*.h"
		  "src/x11/*.c"
		)
elseif(APPLE)
	set(UIOHOOK_SRC
		  "include/uiohook.h"
		  "src/logger.c"
		  "src/logger.h"
		  "src/darwin/*.h"
		  "src/darwin/*.c"
		)
else()
	error("unknown OS")
	return()

endif()


#library 
add_library("uiohook" SHARED ${UIOHOOK_SRC})
if(WIN32 OR WIN64)
	target_include_directories("uiohook" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src" 														"${CMAKE_CURRENT_SOURCE_DIR}/windows")

	if(MSVC_VERSION LESS 1900)
		SET_TARGET_PROPERTIES("uiohook"  PROPERTIES COMPILE_FLAGS "-Dinline=__inline -D_CRT_SECURE_NO_WARNINGS -Dsnprintf=_snprintf")
	else()
		SET_TARGET_PROPERTIES("uiohook"  PROPERTIES COMPILE_FLAGS "-Dinline=__inline -D_CRT_SECURE_NO_WARNINGS")
	endif()
endif()

#demo_hook
add_executable("demo_hook" "${CMAKE_CURRENT_SOURCE_DIR}/src/demo_hook.c")
add_dependencies("demo_hook" "uiohook")
target_link_libraries("demo_hook" "uiohook") 
if(WIN32 OR WIN64)
	if(MSVC_VERSION LESS 1900)
		SET_TARGET_PROPERTIES("uiohook"  PROPERTIES COMPILE_FLAGS "-Dinline=__inline -D_CRT_SECURE_NO_WARNINGS -Dsnprintf=_snprintf")
	else()
		SET_TARGET_PROPERTIES("uiohook"  PROPERTIES COMPILE_FLAGS "-Dinline=__inline -D_CRT_SECURE_NO_WARNINGS")
	endif()
endif()

#demo_hook_async
add_executable("demo_hook_async" "${CMAKE_CURRENT_SOURCE_DIR}/src/demo_hook_async.c")
add_dependencies("demo_hook_async" "uiohook")
target_link_libraries("demo_hook_async" "uiohook") 
if(WIN32 OR WIN64)
	if(MSVC_VERSION LESS 1900)
		SET_TARGET_PROPERTIES("uiohook"  PROPERTIES COMPILE_FLAGS "-Dinline=__inline -D_CRT_SECURE_NO_WARNINGS -Dsnprintf=_snprintf")
	else()
		SET_TARGET_PROPERTIES("uiohook"  PROPERTIES COMPILE_FLAGS "-Dinline=__inline -D_CRT_SECURE_NO_WARNINGS")
	endif()
endif()

#demo_post
add_executable("demo_post" "${CMAKE_CURRENT_SOURCE_DIR}/src/demo_post.c")
add_dependencies("demo_post" "uiohook")
target_link_libraries("demo_post" "uiohook") 
if(WIN32 OR WIN64)
	if(MSVC_VERSION LESS 1900)
		SET_TARGET_PROPERTIES("uiohook"  PROPERTIES COMPILE_FLAGS "-Dinline=__inline -D_CRT_SECURE_NO_WARNINGS -Dsnprintf=_snprintf")
	else()
		SET_TARGET_PROPERTIES("uiohook"  PROPERTIES COMPILE_FLAGS "-Dinline=__inline -D_CRT_SECURE_NO_WARNINGS")
	endif()
endif()

#demo_properties
add_executable("demo_properties" "${CMAKE_CURRENT_SOURCE_DIR}/src/demo_properties.c")
add_dependencies("demo_properties" "uiohook")
target_link_libraries("demo_properties" "uiohook") 
if(WIN32 OR WIN64)
	if(MSVC_VERSION LESS 1900)
		SET_TARGET_PROPERTIES("uiohook"  PROPERTIES COMPILE_FLAGS "-Dinline=__inline -D_CRT_SECURE_NO_WARNINGS -Dsnprintf=_snprintf")
	else()
		SET_TARGET_PROPERTIES("uiohook"  PROPERTIES COMPILE_FLAGS "-Dinline=__inline -D_CRT_SECURE_NO_WARNINGS")
	endif()
endif()


#all demo
add_custom_target("all_demo" 
					DEPENDS
						 "demo_hook"
						 "demo_hook_async"
						 "demo_post"
						 "demo_properties"
		)		
