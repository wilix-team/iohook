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

#include <stdint.h>
#include <stdio.h>
#include <uiohook.h>

#include "minunit.h"

static char * test_auto_repeat_rate() {
	long int i = hook_get_auto_repeat_rate();
	
	fprintf(stdout, "Auto repeat rate: %li\n", i);
	mu_assert("error, could not determine auto repeat rate", i >= 0);
	
	return NULL;
}

static char * test_auto_repeat_delay() {
	long int i = hook_get_auto_repeat_delay();
	
	fprintf(stdout, "Auto repeat delay: %li\n", i);
	mu_assert("error, could not determine auto repeat delay", i >= 0);
	
	return NULL;
}

static char * test_pointer_acceleration_multiplier() {
	long int i = hook_get_pointer_acceleration_multiplier();
	
	fprintf(stdout, "Pointer acceleration multiplier: %li\n", i);
	mu_assert("error, could not determine pointer acceleration multiplier", i >= 0);
	
	return NULL;
}

static char * test_pointer_acceleration_threshold() {
	long int i = hook_get_pointer_acceleration_threshold();
	
	fprintf(stdout, "Pointer acceleration threshold: %li\n", i);
	mu_assert("error, could not determine pointer acceleration threshold", i >= 0);
	
	return NULL;
}

static char * test_pointer_sensitivity() {
	long int i = hook_get_pointer_sensitivity();
	
	fprintf(stdout, "Pointer sensitivity: %li\n", i);
	mu_assert("error, could not determine pointer sensitivity", i >= 0);
	
	return NULL;
}

static char * test_multi_click_time() {
	long int i = hook_get_multi_click_time();
	
	fprintf(stdout, "Multi click time: %li\n", i);
	mu_assert("error, could not determine multi click time", i >= 0);
	
	return NULL;
}

char * system_properties_tests() {
	mu_run_test(test_auto_repeat_rate);
	mu_run_test(test_auto_repeat_delay);
	
	mu_run_test(test_pointer_acceleration_multiplier);
	mu_run_test(test_pointer_acceleration_threshold);
	mu_run_test(test_pointer_sensitivity);
	
	mu_run_test(test_multi_click_time);
	
	return NULL;
}
