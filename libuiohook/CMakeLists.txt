# libUIOHook: Cross-platform keyboard and mouse hooking from userland.
# Copyright (C) 2006-2022 Alexander Barker.  All Rights Reserved.
# https://github.com/kwhat/libuiohook/
#
# libUIOHook is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# libUIOHook is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.10)

project(uiohook VERSION 1.2.0 LANGUAGES C)


if (WIN32 OR WIN64)
    set(UIOHOOK_SOURCE_DIR "windows")
elseif (APPLE)
    set(UIOHOOK_SOURCE_DIR "darwin")
else()
    set(UIOHOOK_SOURCE_DIR "x11")
endif()

add_library(uiohook
    "src/logger.c"
    "src/${UIOHOOK_SOURCE_DIR}/input_helper.c"
    "src/${UIOHOOK_SOURCE_DIR}/input_hook.c"
    "src/${UIOHOOK_SOURCE_DIR}/post_event.c"
    "src/${UIOHOOK_SOURCE_DIR}/system_properties.c"
)

set_target_properties(uiohook PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE 1
    OUTPUT_NAME "${PROJECT_NAME}"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/uiohook.h
)

include(GNUInstallDirs)
target_include_directories(uiohook
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/${UIOHOOK_SOURCE_DIR}
)


install(TARGETS uiohook
    EXPORT ${PROJECT_NAME}-config
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

export(TARGETS uiohook FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake")
install(EXPORT ${PROJECT_NAME}-config DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})


if (BUILD_DEMO)
    if (NOT BUILD_SHARED_LIBS)
        set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    endif()

    find_package(Threads REQUIRED)

    add_executable(demo_hook "./demo/demo_hook.c")
    add_dependencies(demo_hook uiohook)
    target_link_libraries(demo_hook uiohook "${CMAKE_THREAD_LIBS_INIT}")

    add_executable(demo_hook_async "./demo/demo_hook_async.c")
    add_dependencies(demo_hook_async uiohook)
    target_link_libraries(demo_hook_async uiohook "${CMAKE_THREAD_LIBS_INIT}")

    add_executable(demo_post "./demo/demo_post.c")
    add_dependencies(demo_post uiohook)
    target_link_libraries(demo_post uiohook "${CMAKE_THREAD_LIBS_INIT}")

    add_executable(demo_properties "./demo/demo_properties.c")
    add_dependencies(demo_properties uiohook)
    target_link_libraries(demo_properties uiohook "${CMAKE_THREAD_LIBS_INIT}")

    add_custom_target(all_demos DEPENDS
        demo_hook
        demo_hook_async
        demo_post
        demo_properties
    )

    set_target_properties(all_demos PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
    )

    if(MSVC)
        add_compile_definitions(all_demos PRIVATE inline=__inline)
        add_compile_definitions(all_demos PRIVATE _CRT_SECURE_NO_WARNINGS)

        if (MSVC_VERSION LESS "1900")
            add_compile_definitions(all_demos PRIVATE snprintf=_snprintf)
        endif()
    endif()

    install(TARGETS demo_hook demo_hook_async demo_post demo_properties RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(ENABLE_TEST)
    add_executable(uiohook_tests
        "./test/input_helper_test.c"
        "./test/system_properties_test.c"
        "./test/minunit.h"
        "./test/uiohook_test.c"
    )

    target_include_directories(uiohook_tests PRIVATE "./src/${UIOHOOK_SOURCE_DIR}")
    target_link_libraries(uiohook_tests uiohook "${CMAKE_THREAD_LIBS_INIT}")
endif()


if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(X11 REQUIRED x11)
    target_include_directories(uiohook PRIVATE "${X11_INCLUDE_DIRS}")
    target_link_libraries(uiohook "${X11_LDFLAGS}")

    pkg_check_modules(XTST REQUIRED xtst)
    target_include_directories(uiohook PRIVATE "${XTST_INCLUDE_DIRS}")
    target_link_libraries(uiohook "${XTST_LDFLAGS}")

    include(CheckLibraryExists)
    check_library_exists(Xtst XRecordQueryVersion "" HAVE_XRECORD)

    include(CheckIncludeFile)
    check_include_file(X11/extensions/record.h HAVE_RECORD_H "-include X11/Xlib.h")

    option(USE_XKB_COMMON "X Keyboard Common Extension (default: ON)" ON)
    if(USE_XKB_COMMON)
        pkg_check_modules(XKB_COMMON REQUIRED xkbcommon-x11)
        add_compile_definitions(uiohook PRIVATE USE_XKB_COMMON)
        target_include_directories(uiohook PRIVATE "${XKB_COMMON_INCLUDE_DIRS}")
        target_link_libraries(uiohook "${XKB_COMMON_LDFLAGS}")

        pkg_check_modules(X11_XCB REQUIRED x11-xcb)
        target_include_directories(uiohook PRIVATE "${X11_XCB_INCLUDE_DIRS}")
        target_link_libraries(uiohook "${X11_XCB_LDFLAGS}")
    endif()

    option(USE_XKB_FILE "X Keyboard File Extension (default: ON)" ON)
    if(USE_XKB_FILE)
        pkg_check_modules(XKB_FILE REQUIRED xkbfile)
        add_compile_definitions(uiohook PRIVATE USE_XKB_FILE)
        target_include_directories(uiohook PRIVATE "${XKB_FILE_INCLUDE_DIRS}")
        target_link_libraries(uiohook "${XKB_FILE_LDFLAGS}")
    endif()

    option(USE_XT "X Toolkit Extension (default: ON)" ON)
    if(USE_XT)
        pkg_check_modules(XT REQUIRED xt)
        add_compile_definitions(uiohook PRIVATE USE_XT)
        target_include_directories(uiohook PRIVATE "${XT_INCLUDE_DIRS}")
        target_link_libraries(uiohook "${XT_LDFLAGS}")
    endif()


    option(USE_XF86MISC "XFree86-Misc X Extension (default: OFF)" OFF)
    if(USE_XF86MISC)
        pkg_check_modules(XF86MISC REQUIRED Xxf86misc)
        add_compile_definitions(uiohook PRIVATE USE_XF86MISC)
        target_include_directories(uiohook PRIVATE "${XF86MISC_INCLUDE_DIRS}")
        target_link_libraries(uiohook "${XF86MISC_LDFLAGS}")
    endif()

    option(USE_XRANDR "XRandR Extension (default: OFF)" OFF)
    if(USE_XRANDR)
        pkg_check_modules(XRANDR REQUIRED xrandr)
        add_compile_definitions(uiohook PRIVATE USE_XRANDR)
        target_include_directories(uiohook PRIVATE "${XRANDR_INCLUDE_DIRS}")
        target_link_libraries(uiohook "${XRANDR_LDFLAGS}")
    endif()

    option(USE_XINERAMA "Xinerama Extension (default: ON)" ON)
    if(USE_XINERAMA)
        pkg_check_modules(XINERAMA REQUIRED xinerama)
        add_compile_definitions(uiohook PRIVATE USE_XINERAMA)
        target_include_directories(uiohook PRIVATE "${XINERAMA_INCLUDE_DIRS}")
        target_link_libraries(uiohook "${XINERAMA_LDFLAGS}")
    endif()

    option(USE_XRECORD_ASYNC "XRecord Asynchronous API (default: OFF)" OFF)
    if(USE_XRECORD_ASYNC)
        add_compile_definitions(uiohook PRIVATE USE_XRECORD_ASYNC)
    endif()

    option(USE_XTEST "XTest API (default: ON)" ON)
    if(USE_XTEST)
        # XTest API is provided by Xtst
        add_compile_definitions(uiohook PRIVATE USE_XTEST)
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        option(USE_EVDEV "Generic Linux input driver (default: ON)" ON)
        if(USE_EVDEV)
            add_compile_definitions(uiohook PRIVATE USE_EVDEV)
        endif()
    endif()
elseif(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.5")

    find_package(Threads REQUIRED)
    target_link_libraries(uiohook "${CMAKE_THREAD_LIBS_INIT}")

    find_library(CARBON Carbon REQUIRED)
    target_include_directories(uiohook PRIVATE "${CARBON}")
    target_link_libraries(uiohook "${CARBON}")

    option(USE_APPLICATION_SERVICES "ApplicationServices framework (default: ON)" ON)
    if(USE_APPLICATION_SERVICES)
        find_library(APPLICATION_SERVICES ApplicationServices REQUIRED)
        add_compile_definitions(USE_APPLICATION_SERVICES)
        target_include_directories(uiohook PRIVATE "${APPLICATION_SERVICES}")
        target_link_libraries(uiohook "${APPLICATION_SERVICES}")
    endif()

    option(USE_IOKIT "IOKit framework (default: ON)" ON)
    if(USE_IOKIT)
        find_library(IOKIT IOKit REQUIRED)
        add_compile_definitions(USE_IOKIT)
        target_include_directories(uiohook PRIVATE "${IOKIT}")
        target_link_libraries(uiohook "${IOKIT}")
    endif()

    # FIXME Change USE_OBJC flag to USE_APPKIT
    #option(USE_APPKIT "AppKit framework (default: ON)" ON)
    option(USE_OBJC "Objective-C API (default: ON)" ON)
    if(USE_OBJC)
        # FIXME Drop USE_OBJC as it is included in AppKit
        find_library(OBJC objc REQUIRED)
        add_compile_definitions(USE_OBJC)
        target_include_directories(uiohook PRIVATE "${OBJC}")
        target_link_libraries(uiohook "${OBJC}")

        find_library(APPKIT AppKit REQUIRED)
        add_compile_definitions(USE_APPKIT)
        target_include_directories(uiohook PRIVATE "${APPKIT}")
        target_link_libraries(uiohook "${APPKIT}")
    endif()

    option(USE_CARBON_LEGACY "Legacy Carbon framework functionality (default: OFF)" OFF)
    if(USE_CARBON_LEGACY)
        message(DEPRECATION "Legacy Carbon functionality has been deprecated.")
        add_compile_definitions(USE_CARBON_LEGACY)

        if(USE_CARBON_LEGACY AND CMAKE_SIZEOF_VOID_P EQUAL 8)
            message(WARNING "Legacy Carbon functionality should not be used with 64-bit targets.")
        endif()
    endif()
elseif(WIN32)
    target_link_libraries(uiohook Advapi32)
endif()


list(REMOVE_DUPLICATES INTERFACE_LINK_LIBRARIES)
string(REPLACE ";" " " COMPILE_LIBRARIES "${INTERFACE_LINK_LIBRARIES}")
configure_file("pc/uiohook.pc.in" "${PROJECT_BINARY_DIR}/uiohook.pc" @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/uiohook.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
