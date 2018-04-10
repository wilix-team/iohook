/* libUIOHook: Cross-platfrom userland keyboard and mouse hooking.
 * Copyright (C) 2006-2017 Alexander Barker.  All Rights Received.
 * https://github.com/kwhat/libuiohook/
 *
 * libUIOHook is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libUIOHook is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>

#if !defined(__APPLE__) && !defined(__MACH__) && !defined(_WIN32)
#include <X11/Xlib.h>
#endif

#include "input_helper.h"
#include "minunit.h"

extern char * system_properties_tests();
extern char * input_helper_tests();

#if !defined(__APPLE__) && !defined(__MACH__) && !defined(_WIN32)
static Display *disp;
#endif

int tests_run = 0;

static char * init_tests() {
	#if !defined(__APPLE__) && !defined(__MACH__) && !defined(_WIN32)
	// TODO Create our own AC_DEFINE for this value.  Currently defaults to X11 platforms.
	Display *disp = XOpenDisplay(XDisplayName(NULL));
	mu_assert("error, could not open X display", disp != NULL);
	
	load_input_helper(disp);
	#else
	load_input_helper();
	#endif

	return NULL;
}

static char * cleanup_tests() {
	#if !defined(__APPLE__) && !defined(__MACH__) && !defined(_WIN32)
	if (disp != NULL) {
		XCloseDisplay(disp);
		disp = NULL;
	}
	#else
	unload_input_helper();
	#endif

	return NULL;
}

static char * all_tests() {
	mu_run_test(init_tests);
	
	mu_run_test(system_properties_tests);
	mu_run_test(input_helper_tests);
	
	mu_run_test(cleanup_tests);

	return NULL;
}

int main() {
	int status = 1;
	
	char *result = all_tests();
	if (result != NULL) {
		status = 0;
		printf("%s\n", result);
	}
	else {
		printf("ALL TESTS PASSED\n");
	}
	printf("Tests run: %d\n", tests_run);

	return status;
}
