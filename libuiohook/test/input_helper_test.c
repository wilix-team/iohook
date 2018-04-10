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

#include "input_helper.h"
#include "minunit.h"
#include "uiohook.h"

/* Make sure all native keycodes map to virtual scancodes */
static char * test_bidirectional_keycode() {
	for (unsigned short i = 0; i < 256; i++) {
		printf("Testing keycode\t\t\t%3u\t[0x%04X]\n", i, i);

		#ifdef _WIN32
		if ((i > 6 && i < 16) || i > 18) {
		#endif
			// Lookup the virtual scancode...
			uint16_t scancode = keycode_to_scancode(i);
			printf("\tproduced scancode\t%3u\t[0x%04X]\n", scancode, scancode);

			// Lookup the native keycode...
			uint16_t keycode = (uint16_t) scancode_to_keycode(scancode);
			printf("\treproduced keycode\t%3u\t[0x%04X]\n", keycode, keycode);

			// If the returned virtual scancode > 127, we used an offset to
			// calculate the keycode index used above.
			if (scancode > 127) {
				printf("\t\tusing offset\t%3u\t[0x%04X]\n", (scancode & 0x7F) | 0x80, (scancode & 0x7F) | 0x80);
			}

			printf("\n");

			if (scancode != VC_UNDEFINED) {
				mu_assert("error, scancode to keycode failed to convert back", i == keycode);
			}
		#ifdef _WIN32
		}
		#endif
	}

	return NULL;
}

/* Make sure all virtual scancodes map to native keycodes */
static char * test_bidirectional_scancode() {
	for (unsigned short i = 0; i < 256; i++) {
		printf("Testing scancode\t\t%3u\t[0x%04X]\n", i, i);

		// Lookup the native keycode...
		uint16_t keycode = (uint16_t) scancode_to_keycode(i);
		printf("\treproduced keycode\t%3u\t[0x%04X]\n", keycode, keycode);

		// Lookup the virtual scancode...
		uint16_t scancode = keycode_to_scancode(keycode);
		printf("\tproduced scancode\t%3u\t[0x%04X]\n", scancode, scancode);

		// If the returned virtual scancode > 127, we used an offset to
		// calculate the keycode index used above.
		if (scancode > 127) {
			// Fix the scancode for upper offsets.
			scancode = (scancode & 0x7F) | 0x80;
			printf("\t\tusing offset\t%3u\t[0x%04X]\n", scancode, scancode);
		}

		printf("\n");
		
		#if defined(__APPLE__) && defined(__MACH__)
		if (keycode != 255) {
		#else
		if (keycode != 0x0000) {
		#endif	
			mu_assert("error, scancode to keycode failed to convert back", i == scancode);
		}
	}

	return NULL;
}

char * input_helper_tests() {
	mu_run_test(test_bidirectional_keycode);
	mu_run_test(test_bidirectional_scancode);

	return NULL;
}
