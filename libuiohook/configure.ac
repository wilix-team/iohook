# libUIOHook: Cross-platfrom userland keyboard and mouse hooking.
# Copyright (C) 2006-2017 Alexander Barker.  All Rights Received.
# https://github.com/kwhat/libuiohook/
#
# libUIOHook is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# libUIOHook is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# Define a few constants using m4 macros to prevent multiple definitions.
m4_define([UIOHOOK_MAJOR],	[1])
m4_define([UIOHOOK_MINOR],	[1])
m4_define([UIOHOOK_PATCH],	[0])
m4_define([UIOHOOK_RC],		[])
m4_define([UIOHOOK_BUGS],	[alex@1stleg.com])
m4_define([UIOHOOK_VERSION_STRING], UIOHOOK_MAJOR[.]UIOHOOK_MINOR[.]UIOHOOK_PATCH[]UIOHOOK_RC)

m4_pattern_allow([AM_PROG_AR])

# Library versioning
# These numbers should be tweaked on every release. Read carefully:
# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
# http://sourceware.org/autobook/autobook/autobook_91.html
lt_current="1"
lt_revision="0"
lt_age="0"
LTLDFLAGS="-version-info ${lt_current}:${lt_revision}:${lt_age}"


# Initialize autoconf.
AC_INIT([uiohook], [UIOHOOK_VERSION_STRING], [UIOHOOK_BUGS])

AC_PREREQ(2.65)

AC_CONFIG_SRCDIR([include/uiohook.h])
AC_CONFIG_FILES([
	pc/uiohook.pc \
	Makefile
])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([include/config.h:include/config.h.in])
AC_CONFIG_MACRO_DIR([m4])

#AC_CANONICAL_SYSTEM
AC_CANONICAL_HOST
AC_CANONICAL_BUILD
AC_CANONICAL_TARGET

# Initialize automake.
AM_INIT_AUTOMAKE([foreign subdir-objects])

# Checks for programs.
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Checks for language.
AC_LANG([C])
AC_PROG_CC_C99

# Checks for header files
AC_C_CONST
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([stdint.h])
AC_TYPE_UINT64_T

# Initialize libtool.
AC_PROG_LIBTOOL

# Check for operating system.
AC_MSG_CHECKING(operating system)
AS_CASE([$host],
	[*-darwin*], [
		AC_MSG_RESULT([$host (Mac OS X Compatible)])
		backend="darwin"
	],
	[*-linux*], [
		AC_MSG_RESULT([$host (Linux Compatible)])
		backend="x11"
		has_evdev="1"
	],
	[*-freebsd* | *-openbsd* | *-linux* | *-solaris*], [
		AC_MSG_RESULT([$host (Unix Compatible)])
		backend="x11"
		has_evdev="0"
	],
	[*-mingw* | *-cygwin* | *-msys* ], [
		AC_MSG_RESULT([$host (Windows Compatible)])
		backend="windows"

		AC_LIBTOOL_WIN32_DLL
	],
	[AC_MSG_ERROR([Unsupported operating system ${host}])])


# Common Options
AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug], [Enable debug output (default: disabled)]),
	[enable_debug="$enableval"],
	[enable_debug="no"])

AC_ARG_ENABLE([demo],
	AS_HELP_STRING([--enable-demo], [Enable demo application (default: disabled)]),
	[enable_demo=$enableval],
	[enable_demo="no"])

AC_ARG_ENABLE([test],
	AS_HELP_STRING([--enable-test], [Enable unit testing (default: disabled)]),
	[enable_test=$enableval],
	[enable_test="no"])

AC_ARG_ENABLE([quiet],
	AS_HELP_STRING([--enable-quiet], [Enable copyright suppression (default: disabled)]),
	[enable_quiet=$enableval],
	[enable_quiet="no"])

# Linux Options
AC_ARG_ENABLE([evdev],
	AS_HELP_STRING([--without-evdev], [Disable generic Linux input driver (default: enabled)]),
	[enable_evdev="$enableval"],
	[enable_evdev="yes"])

# Linux & Unix Options
AC_ARG_ENABLE([xkbcommon],
	AS_HELP_STRING([--without-xkbcommon], [Disable X Keyboard Common Extension (default: enabled)]),
	[enable_xkbcommon="$enableval"],
	[enable_xkbcommon="yes"])
	
AC_ARG_ENABLE([xkbfile],
	AS_HELP_STRING([--without-xkbfile], [Disable X Keyboard File Extension (default: enabled)]),
	[enable_xkbfile="$enableval"],
	[enable_xkbfile="yes"])

AC_ARG_ENABLE([xcb],
	AS_HELP_STRING([--without-xcb], [Disable X protocol C-language Binding (default: enabled)]),
	[enable_xcb="$enableval"],
	[enable_xcb="yes"])

AC_ARG_ENABLE([xkb],
	AS_HELP_STRING([--without-xkb], [Disable X Keyboard Extension (default: enabled)]),
	[enable_xkb="$enableval"],
	[enable_xkb="yes"])

AC_ARG_ENABLE([xt],
	AS_HELP_STRING([--without-xt], [Disable X Toolkit Intrinsics (default: enabled)]),
	[enable_xt="$enableval"],
	[enable_xt="yes"])

AC_ARG_ENABLE([xf86misc],
	AS_HELP_STRING([--with-xf86misc], [Enable XFree86-Misc X Extension (default: disabled)]),
	[enable_xf86misc="$enableval"],
	[enable_xf86misc="no"])

AC_ARG_ENABLE([xrecord-async],
	AS_HELP_STRING([--enable-xrecord-async], [Enable XRecord Asynchronous API (default: disabled)]),
	[enable_xrecord_async="$enableval"],
	[enable_xrecord_async="no"])

AC_ARG_ENABLE([xtest],
	AS_HELP_STRING([--without-xtest], [Disable XTest Extension (default: enabled)]),
	[enable_xtest="$enableval"],
	[enable_xtest="yes"])

AC_ARG_ENABLE([xinerama],
	AS_HELP_STRING([--without-xinerama], [Disable Xinerama Extension (default: enabled)]),
	[enable_xinerama="$enableval"],
	[enable_xinerama="yes"])

AC_ARG_ENABLE([xrandr],
	AS_HELP_STRING([--with-xrandr], [Enable XRandR Extension (default: disabled)]),
	[enable_xrandr="$enableval"],
	[enable_xrandr="no"])

# Darwin Options
AC_ARG_ENABLE([corefoundation],
	AS_HELP_STRING([--without-corefoundation],	[Disable CoreFoundation framework (default: enabled)]),
	[enable_corefoundation="$enableval"],
	[enable_corefoundation="yes"])

AC_ARG_ENABLE([iokit],
	AS_HELP_STRING([--without-iokit],	[Disable IOKit framework (default: enabled)]),
	[enable_iokit="$enableval"],
	[enable_iokit="yes"])

AC_ARG_ENABLE([objc],
	AS_HELP_STRING([--without-objc],	[Disable objective-c (default: enabled)]),
	[enable_objc="$enableval"],
	[enable_objc="yes"])

AC_ARG_ENABLE([weak-import],
	AS_HELP_STRING([--enable-weak-import], [Enable support for weakly-linked symbols (default: disabled)]),
	[enable_weak_import="$enableval"],
	[enable_weak_import="no"])

AC_ARG_ENABLE([carbon-legacy],
	AS_HELP_STRING([--with-carbon-legacy], [Enable legacy Carbon framework functionality (default: disabled)]),
	[enable_carbon_legacy="$enableval"],
	[enable_carbon_legacy="no"])

AS_IF([test "x$enable_debug" = "xyes"], [
	AC_DEFINE([USE_DEBUG], 1, [Enable Debug Output])
])

AS_IF([test "x$enable_quiet" = "xyes"], [
	AC_DEFINE([USE_QUIET], 1, [Enable Copyright Suppression])
])

AS_IF(
	[test "x$backend" = "xdarwin"], [
		# Force CLANG for OS X because it is required as of OS X 10.10.
		AC_PROG_CC([clang])

		AS_IF([test "x$enable_corefoundation" = "xyes"], [
			AC_DEFINE([USE_COREFOUNDATION], 1, [Enable CoreFoundation framework])
			LIBS="${LIBS} -framework ApplicationServices"
		])

		AS_IF([test "x$enable_iokit" = "xyes"], [
			AC_DEFINE([USE_IOKIT], 1, [Enable IOKit framework])
			LIBS="${LIBS} -framework IOKit"
		])

		AS_IF([test "x$enable_objc" = "xyes"], [
			AC_DEFINE([USE_OBJC], 1, [Enable objective-c])
			LIBS="${LIBS} -lobjc"
		])

		AS_IF([test "x$enable_weak_import" = "xyes"], [
			AC_DEFINE([USE_WEAK_IMPORT], 1, [Enable support for weakly-linked symbols])
			LDFLAGS="$LDFLAGS -Wl,-flat_namespace,-undefined,dynamic_lookup"
		])

		AS_IF([test "x$enable_carbon_legacy" = "xyes"], [
			# FIXME AC_MSG_ERROR if this is a 64 bit target.
			AC_MSG_WARN([Carbon legacy functionality is required for OS X version 10.4 and should only be used on 32-bit targets.])
			AC_DEFINE([USE_CARBON_LEGACY], 1, [Enable Carbon framework legacy functionality])
		])

		AC_DEFINE([MAC_OS_X_VERSION_MIN_REQUIRED], 1050, [Set minimum OS X version to 10.5])

		# We only need pthreads if we are using the Demo.
		AS_IF([test "x$enable_demo" = "xyes"], [
			AX_PTHREAD([found_pthreads=yes], [found_pthreads=no])

			AS_IF([test "x$found_pthread" = "xno" ], [
				AC_MSG_ERROR([Pthreads are required for Async XRecord, XRandR and the demo!])
			])

			# FIXME For some reason, llvm will not use the pthread flag.
			#THREAD_LIBS="$PTHREAD_LIBS $LIBS"
			#THREAD_CFLAGS="$PTHREAD_CFLAGS $CFLAGS"
		])

		# Always add Carbon due to non-deprecated #define's
		LIBS="${LIBS} -framework Carbon"

		#CFLAGS="$CFLAGS -I/usr/local/include"
		#LDFLAGS="$LDFLAGS -Wl,-undefined,error"
	],
	[test "x$backend" = "xx11"], [
		# Checks for libraries.
		PKG_PROG_PKG_CONFIG

		PKG_CHECK_MODULES([X11], [x11])
		LIBS="$X11_LIBS $LIBS"
		#CFLAGS="$X11_CFLAGS $CFLAGS"
		REQUIRE="$REQUIRE x11"

		PKG_CHECK_MODULES([XTST], [xtst])
		LIBS="$XTST_LIBS $LIBS"
		#CFLAGS="$XTST_CFLAGS $CFLAGS"
		REQUIRE="$REQUIRE xtst"

		# Check for XRecord.
		AC_CHECK_LIB([Xtst], [XRecordQueryVersion], [],
			[AC_MSG_ERROR([libXtst does not include XRecord extention!])],
			$LIBS)

		AC_CHECK_HEADERS([X11/extensions/record.h], [],
			[AC_MSG_ERROR([XRecord extention header could not be found!])],
			[#include <X11/Xlib.h>])

		# Evdev is only available on Linux.
		AS_IF([test "x$has_evdev" = "x1"], [
			AS_IF([test "x$enable_evdev" = "xyes"], [
				AC_DEFINE([USE_EVDEV], 1, [Enable generic Linux input driver])
			])
		])

		AS_IF([test "x$enable_xkb" = "xyes"], [
			AC_DEFINE([USE_XKB], 1, [Enable X Keyboard Extension])
		])

		AS_IF([test "x$enable_xt" = "xyes"], [
			AC_DEFINE([USE_XT], 1, [Enable X Toolkit Intrinsics])
			PKG_CHECK_MODULES([XT], [xt])
			#CFLAGS="$XT_CFLAGS $CFLAGS"
			LIBS="$XT_LIBS $LIBS"
			REQUIRE="$REQUIRE xt"
		])

		AS_IF([test "x$enable_xf86misc" = "xyes"], [
			AC_DEFINE([USE_XF86MISC], 1, [Enable XFree86-Misc X Extension])
			PKG_CHECK_MODULES([XF86MISC], [xxf86misc])
			#CFLAGS="$XF86MISC_CFLAGS $CFLAGS"
			LIBS="$XF86MISC_LIBS $LIBS"
			REQUIRE="$REQUIRE xxf86misc"
		])

		AS_IF([test "x$enable_xrecord_async" = "xyes"], [
			AC_DEFINE([USE_XRECORD_ASYNC], 1, [Enable XRecord Asynchronous API])
			CFLAGS="$CFLAGS -D_POSIX_C_SOURCE=199309L"
		])

		AS_IF([test "x$enable_xtest" = "xyes"], [
			AC_CHECK_LIB([Xtst], [XTestQueryExtension],
				[AC_CHECK_HEADERS([X11/extensions/XTest.h],
						[AC_DEFINE([USE_XTEST], 1, [Enable XTest Extension])],
						[AC_MSG_WARN([XTest extention header could not be found!])],
						[#include <X11/Xlib.h>])],
				[AC_MSG_WARN([libXtst does not include XTest extention!])],
				$LIBS)
		])

		AS_IF([test "x$enable_xrandr" = "xyes"], [
			AS_IF([test "x$enable_xinerama" = "xyes" ], [
				AC_MSG_WARN([Both Xinerama and XRandR were enabled, ignoring Xinerama!])
			])

			AC_DEFINE([USE_XRANDR], 1, [Enable XRandR Extension])
			PKG_CHECK_MODULES([XRANDR], [xrandr])
			LIBS="$XRANDR_LIBS $LIBS"
			#CFLAGS="$XRANDR_CFLAGS $CFLAGS"
			REQUIRE="$REQUIRE xrandr"
		],[
			AS_IF([test "x$enable_xinerama" = "xyes"], [
				AC_DEFINE([USE_XINERAMA], 1, [Enable Xinerama Extension])
				PKG_CHECK_MODULES([XINERAMA], [xinerama])
				LIBS="$XINERAMA_LIBS $LIBS"
				#CFLAGS="$XINERAMA_CFLAGS $CFLAGS"
				REQUIRE="$REQUIRE xinerama"
			])
		])

		# We only need pthreads if we are using XRandR, Async XRecord or the Demo.
		AS_IF([test "x$enable_xrecord_async" = "xyes" || test "x$enable_xrandr" = "xyes" || test "x$enable_demo" = "xyes"], [
			AX_PTHREAD([found_pthreads=yes], [found_pthreads=no])

			AS_IF([test "x$found_pthread" = "xno" ], [
				AC_MSG_ERROR([Pthreads are required for Async XRecord, XRandR and the demo!])
			])

			AS_IF([test "x$enable_xrecord_async" = "xyes" || test "x$enable_xrandr" = "xyes"], [
				LIBS="$PTHREAD_LIBS $LIBS"
				CFLAGS="$PTHREAD_CFLAGS $CFLAGS"
			], [test "x$enable_demo" = "xyes"], [
				DEMO_LIBS="$PTHREAD_LIBS $LIBS"
				DEMO_CFLAGS="$PTHREAD_CFLAGS $CFLAGS"
			])
		])


		AS_IF([test "x$enable_xkbcommon" = "xyes"], [
			PKG_CHECK_MODULES([X11_XCB], [x11-xcb])
			LIBS="$X11_XCB_LIBS $LIBS"
			CFLAGS="$X11_XCB_CFLAGS $CFLAGS"
			REQUIRE="$REQUIRE x11-xcb"

			PKG_CHECK_MODULES([XKBCOMMON], [xkbcommon])
			LIBS="$XKBCOMMON_LIBS $LIBS"
			CFLAGS="$XKBCOMMON_CFLAGS $CFLAGS"
			REQUIRE="$REQUIRE xkbcommon"

			PKG_CHECK_MODULES([XKBCOMMON_X11], [xkbcommon-x11])
			LIBS="$XKBCOMMON_X11_LIBS $LIBS"
			#CFLAGS="$XKBCOMMON_X11_CFLAGS $CFLAGS"
			REQUIRE="$REQUIRE xkbcommon-x11"

			AC_DEFINE([USE_XKBCOMMON], 1, [Enable XKB Common Extension])
		])

		AS_IF([test "x$enable_xkbfile" = "xyes"], [
			AS_IF([test "x$enable_kxbcommon" = "xno"], [
				AC_MSG_ERROR([XKB Common is required for XKB File support!])
			])

			PKG_CHECK_MODULES([XKBFILE], [xkbfile])
			LIBS="$XKBFILE_LIBS $LIBS"
			#CFLAGS="$XKBFILE_CFLAGS $CFLAGS"
			REQUIRE="$REQUIRE xkbfile"

			AC_DEFINE([USE_XKBFILE], 1, [Enable XKB File Extension])
		])

		LDFLAGS="$LDFLAGS -Wl,--no-undefined"
	],
	[test "x$backend" = "xwindows"], [
		AC_DEFINE([_WIN32_WINNT], 0x0500, [Set minimum Windows version to Windows 2000])

		AS_IF([test "x$enable_debug" = "xyes"], [
			CFLAGS="$CFLAGS -Wno-format"
		])

		LDFLAGS="$LDFLAGS  -Wl,--no-undefined -no-undefined"
	])

# OS info for Automake
AC_SUBST([backend])

# Requires for pkg-config
AC_SUBST([REQUIRE])
AC_SUBST([LIBS])
AC_SUBST([DEMO_CFLAGS])
AC_SUBST([DEMO_LIBS])
AC_SUBST([TEST_LIBS])

# Should the demo application be built?
AM_CONDITIONAL([BUILD_DEMO], [test "x$enable_demo" = "xyes"])

# Should the unit tests be built?
AM_CONDITIONAL([BUILD_TEST], [test "x$enable_test" = "xyes"])

# Department of redundancy department.  We have already checked the platform 3
# times!  I am sure there is a technical reason for this...
AM_CONDITIONAL(BUILD_DARWIN, test "x$backend" = "xdarwin")
AM_CONDITIONAL(BUILD_X11, test "x$backend" = "xx11")
AM_CONDITIONAL(BUILD_WINDOWS, test "x$backend" = "xwindows")

# Generate the "configure" script
AC_OUTPUT
