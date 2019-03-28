cmake_minimum_required(VERSION 2.8)

macro(use_c99)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_C_FLAGS "--std=gnu99 ${CMAKE_C_FLAGS}")
    endif ()
  else ()
    set (CMAKE_C_STANDARD 99)
  endif ()
endmacro(use_c99)

use_c99()

set (CMAKE_CXX_STANDARD 11)

project(iohook)

if(WIN32 OR WIN64)
    add_subdirectory(libuiohook ${CMAKE_CURRENT_SOURCE_DIR}/libuiohook)
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")

  #bootstrap and configure
  set(_config_headers "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/include/config.h")
  add_custom_target( "prepare_iuhook"
                      COMMAND "./bootstrap.sh"
                      COMMAND "./configure"
                      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook")

  file(GLOB SOURCE_UIHOOK_FILES "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/src/logger.c"
                                "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/src/logger.h"
                                "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/src/x11/*.c"
                                "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/src/x11/*.h"
                                "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/include/config.h" )

  add_library( "uiohook" STATIC ${SOURCE_UIHOOK_FILES} )
  set_target_properties("uiohook"  PROPERTIES COMPILE_FLAGS "-DHAVE_CONFIG_H=1 -fPIC")
  add_dependencies( "uiohook" "prepare_iuhook")
  target_include_directories("uiohook" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/include/" "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/src" ${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/src/x11 )
else()
  #bootstrap and configure
  set(_config_headers "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/include/config.h")
  add_custom_target( "prepare_iuhook"
                      COMMAND "./bootstrap.sh"
                      COMMAND "./configure"
                      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook")

  file(GLOB SOURCE_UIHOOK_FILES "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/src/logger.c"
                                "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/src/logger.h"
                                "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/src/darwin/*.c"
                                "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/src/darwin/*.h"
                                "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/include/config.h" )

  add_library( "uiohook" STATIC ${SOURCE_UIHOOK_FILES} )
  set_target_properties("uiohook"  PROPERTIES COMPILE_FLAGS "-DHAVE_CONFIG_H=1")
  add_dependencies( "uiohook" "prepare_iuhook")
  target_include_directories("uiohook" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/include/" "${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/src" ${CMAKE_CURRENT_SOURCE_DIR}/libuiohook/src/darwin )

endif()

# Build a shared library named after the project from the files in `src/`
file(GLOB SOURCE_FILES "src/*.cc" "src/*.h")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})

# Gives our library file a .node extension without any "lib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# Essential include files to build a node addon,
# You should add this line in every CMake.js based project
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})

# Essential library files to link to a node addon
# You should add this line in every CMake.js based project
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} "uiohook")

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} "uiohook" "xkbfile" "xkbcommon-x11" "xkbcommon" "X11-xcb" "xcb" "Xinerama" "Xt" "Xtst" "X11")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "(Darwin)")
  find_library(FRAMEWORK_IOKIT IOKit)
  find_library(FRAMEWORK_Carbon Carbon)
  target_link_libraries(${PROJECT_NAME} ${FRAMEWORK_IOKIT} ${FRAMEWORK_Carbon})
endif()
