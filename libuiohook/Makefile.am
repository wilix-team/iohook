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

ACLOCAL_AMFLAGS = -I m4

HOOK_SRC = src/logger.c
TEST_SRC = test/input_helper_test.c test/system_properties_test.c test/uiohook_test.c
MAN3_SRC = man/hook_run.man \
	   man/hook_stop.man \
	   man/hook_get_auto_repeat_delay.man \
	   man/hook_get_multi_click_time.man \
	   man/hook_get_pointer_acceleration_threshold.man \
	   man/hook_set_dispatch_proc.man \
	   man/hook_get_auto_repeat_rate.man \
	   man/hook_get_pointer_acceleration_multiplier.man \
	   man/hook_get_pointer_sensitivity.man \
	   man/hook_set_logger_proc.man

if BUILD_DARWIN
HOOK_SRC += src/darwin/input_helper.c \
	 src/darwin/input_hook.c \
	 src/darwin/post_event.c \
	 src/darwin/system_properties.c
endif

if BUILD_X11
HOOK_SRC += src/x11/input_helper.c \
	 src/x11/input_hook.c \
	 src/x11/post_event.c \
	 src/x11/system_properties.c
endif

if BUILD_WINDOWS
HOOK_SRC += src/windows/input_helper.c \
	 src/windows/input_hook.c \
	 src/windows/post_event.c \
	 src/windows/system_properties.c
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(top_builddir)/pc/uiohook.pc

include_HEADERS = include/uiohook.h

bin_PROGRAMS =
lib_LTLIBRARIES = libuiohook.la


libuiohook_la_CFLAGS = $(AM_CFLAGS) -Wall -Wextra -pedantic -Wno-unused-parameter -I$(top_srcdir)/include -I$(top_srcdir)/src/$(backend) -I$(top_srcdir)/src
libuiohook_la_LDFLAGS = $(LTLDFLAGS) $(LDFLAGS)
libuiohook_la_SOURCES = $(HOOK_SRC)


if BUILD_DEMO
bin_PROGRAMS += demohook demohookasync demopost demoprops

demohook_SOURCES = src/demo_hook.c
demohook_LDADD = $(top_builddir)/libuiohook.la
demohook_CFLAGS = $(AM_CFLAGS) -Wall -Wextra -pedantic $(DEMO_CFLAGS) -I$(top_srcdir)/include
demohook_LDFLAGS = $(LTLDFLAGS) $(DEMO_LIBS)

demohookasync_SOURCES = src/demo_hook_async.c
demohookasync_LDADD = $(top_builddir)/libuiohook.la
demohookasync_CFLAGS = $(AM_CFLAGS) -Wall -Wextra -pedantic $(DEMO_CFLAGS) -I$(top_srcdir)/include
demohookasync_LDFLAGS = $(LTLDFLAGS) $(DEMO_LIBS)

demopost_SOURCES = src/demo_post.c
demopost_LDADD = $(top_builddir)/libuiohook.la
demopost_CFLAGS = $(AM_CFLAGS) -Wall -Wextra -pedantic $(DEMO_CFLAGS) -I$(top_srcdir)/include
demopost_LDFLAGS = $(LTLDFLAGS) $(DEMO_LIBS)

demoprops_SOURCES = src/demo_properties.c
demoprops_LDADD = $(top_builddir)/libuiohook.la
demoprops_CFLAGS = $(AM_CFLAGS) -Wall -Wextra -pedantic $(DEMO_CFLAGS) -I$(top_srcdir)/include
demoprops_LDFLAGS = $(LTLDFLAGS) $(DEMO_LIBS)
endif


if BUILD_TEST
bin_PROGRAMS += testhook

testhook_SOURCES = $(TEST_SRC)
testhook_LDADD = $(top_builddir)/libuiohook.la
testhook_CFLAGS = $(AM_CFLAGS) -Wall -Wextra -pedantic $(TEST_CFLAGS) -I$(top_srcdir)/include -I$(top_srcdir)/test -I$(top_srcdir)/src/$(backend) -I$(top_srcdir)/src
testhook_LDFLAGS = $(LTLDFLAGS) $(TEST_LIBS)
endif

man3_MANS = $(MAN3_SRC)

EXTRA_DIST = COPYING.md COPYING.LESSER.md README.md
