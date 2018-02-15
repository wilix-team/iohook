cmake_minimum_required(VERSION 2.8)

project(libuiohook)

# Common Options
option(ENABLE_DEBUG "Enable debug output (default: disabled)" OFF)
option(ENABLE_DEMO "Enable demo applicaitons (default: disabled)" OFF)
option(ENABLE_TEST "Enable unit testing (default: disabled)" OFF)
option(ENABLE_QUIET "Enable copyright suppression (default: disabled)" OFF)

# libUIOHook Sources.
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
elseif(APPLE)
	set(UIOHOOK_SRC
		  "include/uiohook.h"
		  "src/logger.c"
		  "src/logger.h"
		  "src/darwin/*.h"
		  "src/darwin/*.c"
		)
else()

	if(LINUX)
        option(USE_EVDEV "Disable generic Linux input driver (default: enabled)" ON)
	endif()

	option(USE_XKB "Disable X Keyboard Extension (default: enabled)" ON)
	option(USE_XKBCOMMON "Disable X Keyboard Common Extension (default: enabled)" ON)
	option(USE_XKBFILE "Disable X Keyboard File Extension (default: enabled)" ON)

	option(USE_XT "Disable X Toolkit Intrinsics (default: enabled)" ON)
	option(USE_XTEST "Disable XTest Extension (default: enabled)" ON)
	option(USE_XINERAMA "Disable Xinerama Extension (default: enabled)" ON)

	option(USE_XF86MISC "Enable XFree86-Misc X Extension (default: disabled)" OFF)
	option(USE_XRANDER "Enable XRandR Extension (default: disabled)" OFF)
	option(USE_XRECORD_ASYNC "Enable XRecord Asynchronous API (default: disabled)" OFF)

	find_package(PkgConfig REQUIRED)

	pkg_check_modules(PC_X11 REQUIRED x11)
	pkg_check_modules(PC_XTST REQUIRED xtst)

	include(CheckLibraryExists)
	check_library_exists(Xtst XRecordQueryVersion "" HAVE_XRECORD)

	if (USE_XT)
        pkg_check_modules(PC_XT REQUIRED xt)
    endif()


	find_path(XRecord_H X11/extensions/record.h HINTS ${PC_X11_INCLUDEDIR} ${PC_XTST_INCLUDEDIR})

	# Use pkg-config to get hints about paths
    #pkg_check_modules(Magick++_PKGCONF ImageMagick++)

    # Include dir
    find_path(Magick++_INCLUDE_DIR
    NAMES Magick++.h
    PATHS ${Magick++_PKGCONF_INCLUDE_DIRS}
    )

    # Finally the library itself
    find_library(Magick++_LIBRARY
    NAMES Magick++
    PATHS ${Magick++_PKGCONF_LIBRARY_DIRS}
    )


	find_package(LibXML++ REQUIRED)
    include_directories(${LibXML++_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${LibXML++_LIBRARIES})

	set(UIOHOOK_SRC
		  "${CMAKE_CURRENT_SOURCE_DIR}/include/uiohook.h"
		  "${CMAKE_CURRENT_SOURCE_DIR}/src/logger.c"
		  "${CMAKE_CURRENT_SOURCE_DIR}/src/logger.h"
		  "${CMAKE_CURRENT_SOURCE_DIR}/src/x11/input_helper.h"
		  "${CMAKE_CURRENT_SOURCE_DIR}/src/x11/input_helper.c"
		  "${CMAKE_CURRENT_SOURCE_DIR}/src/x11/input_hook.c"
		  "${CMAKE_CURRENT_SOURCE_DIR}/src/x11/post_event.c"
		  "${CMAKE_CURRENT_SOURCE_DIR}/src/x11/system_properties.c"
		)

endif()

configure_file("${PROJECT_SOURCE_DIR}/include/config.h.in"
		"${CMAKE_CURRENT_BINARY_DIR}/include/config.h")

include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")
add_definitions (-DHAVE_CONFIG_H)

# Library
add_library("uiohook" SHARED ${UIOHOOK_SRC})
target_include_directories("uiohook" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src")
if(WIN32 OR WIN64)
	if(MSVC_VERSION LESS 1900)
		SET_TARGET_PROPERTIES("uiohook"  PROPERTIES COMPILE_FLAGS "-Dinline=__inline -D_CRT_SECURE_NO_WARNINGS -Dsnprintf=_snprintf")
	else()
		SET_TARGET_PROPERTIES("uiohook"  PROPERTIES COMPILE_FLAGS "-Dinline=__inline -D_CRT_SECURE_NO_WARNINGS")
	endif()
endif()

# function to generate demo project
function (register_demo dname)
	add_executable("${dname}" "${CMAKE_CURRENT_SOURCE_DIR}/src/${dname}.c")
	add_dependencies("${dname}" "uiohook")
	target_link_libraries("${dname}" "uiohook")
	if(WIN32 OR WIN64)
		if(MSVC_VERSION LESS 1900)
			SET_TARGET_PROPERTIES("${dname}"  PROPERTIES COMPILE_FLAGS "-Dinline=__inline -D_CRT_SECURE_NO_WARNINGS -Dsnprintf=_snprintf")
		else()
			SET_TARGET_PROPERTIES("${dname}"  PROPERTIES COMPILE_FLAGS "-Dinline=__inline -D_CRT_SECURE_NO_WARNINGS")
		endif()
	endif()
endfunction()

# demo_hook
register_demo("demo_hook")

# demo_hook_async
register_demo("demo_hook_async")

#demo_post
register_demo("demo_post")

# demo_properties
register_demo("demo_properties")

# all demo
add_custom_target("all_demo" 
					DEPENDS
						 "demo_hook"
						 "demo_hook_async"
						 "demo_post"
						 "demo_properties"
		)
