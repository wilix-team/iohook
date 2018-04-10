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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <uiohook.h>

bool logger_proc(unsigned int level, const char *format, ...) {
	bool status = false;
	
	va_list args;
	switch (level) {
		#ifdef USE_DEBUG
		case LOG_LEVEL_DEBUG:
		case LOG_LEVEL_INFO:
			va_start(args, format);
			status = vfprintf(stdout, format, args) >= 0;
			va_end(args);
			break;
		#endif

		case LOG_LEVEL_WARN:
		case LOG_LEVEL_ERROR:
			va_start(args, format);
			status = vfprintf(stderr, format, args) >= 0;
			va_end(args);
			break;
	}
	
	return status;
}

// NOTE: The following callback executes on the same thread that hook_run() is called 
// from.  This is important because hook_run() attaches to the operating systems
// event dispatcher and may delay event delivery to the target application.
// Furthermore, some operating systems may choose to disable your hook if it 
// takes to long to process.  If you need to do any extended processing, please 
// do so by copying the event to your own queued dispatch thread.
void dispatch_proc(uiohook_event * const event) {
	char buffer[256] = { 0 };
	size_t length = snprintf(buffer, sizeof(buffer), 
			"id=%i,when=%" PRIu64 ",mask=0x%X", 
			event->type, event->time, event->mask);
	
	switch (event->type) {
		case EVENT_KEY_PRESSED:
			// If the escape key is pressed, naturally terminate the program.
			if (event->data.keyboard.keycode == VC_ESCAPE) {
				int status = hook_stop();
				switch (status) {
					// System level errors.
					case UIOHOOK_ERROR_OUT_OF_MEMORY:
						logger_proc(LOG_LEVEL_ERROR, "Failed to allocate memory. (%#X)", status);
						break;
			
					case UIOHOOK_ERROR_X_RECORD_GET_CONTEXT:
						// NOTE This is the only platform specific error that occurs on hook_stop().
						logger_proc(LOG_LEVEL_ERROR, "Failed to get XRecord context. (%#X)", status);
						break;

					// Default error.
					case UIOHOOK_FAILURE:
					default:
						logger_proc(LOG_LEVEL_ERROR, "An unknown hook error occurred. (%#X)", status);
						break;
				}
			}
		case EVENT_KEY_RELEASED:
			snprintf(buffer + length, sizeof(buffer) - length, 
				",keycode=%u,rawcode=0x%X",
				event->data.keyboard.keycode, event->data.keyboard.rawcode);
			break;

		case EVENT_KEY_TYPED:
			snprintf(buffer + length, sizeof(buffer) - length, 
				",keychar=%lc,rawcode=%u",
				(wint_t) event->data.keyboard.keychar,
				event->data.keyboard.rawcode);
			break;

		case EVENT_MOUSE_PRESSED:
		case EVENT_MOUSE_RELEASED:
		case EVENT_MOUSE_CLICKED:
		case EVENT_MOUSE_MOVED:
		case EVENT_MOUSE_DRAGGED:
			snprintf(buffer + length, sizeof(buffer) - length, 
				",x=%i,y=%i,button=%i,clicks=%i",
				event->data.mouse.x, event->data.mouse.y,
				event->data.mouse.button, event->data.mouse.clicks);
			break;

		case EVENT_MOUSE_WHEEL:
			snprintf(buffer + length, sizeof(buffer) - length, 
				",type=%i,amount=%i,rotation=%i",
				event->data.wheel.type, event->data.wheel.amount,
				event->data.wheel.rotation);
			break;

		default:
			break;
	}

	fprintf(stdout, "%s\n",	 buffer);
}

int main() {
	// Set the logger callback for library output.
	hook_set_logger_proc(&logger_proc);
	
	// Set the event callback for uiohook events.
	hook_set_dispatch_proc(&dispatch_proc);

	// Start the hook and block.
	// NOTE If EVENT_HOOK_ENABLED was delivered, the status will always succeed.
	int status = hook_run();
	switch (status) {
		case UIOHOOK_SUCCESS:
			// Everything is ok.
			break;

		// System level errors.
		case UIOHOOK_ERROR_OUT_OF_MEMORY:
			logger_proc(LOG_LEVEL_ERROR, "Failed to allocate memory. (%#X)", status);
			break;


		// X11 specific errors.
		case UIOHOOK_ERROR_X_OPEN_DISPLAY:
			logger_proc(LOG_LEVEL_ERROR, "Failed to open X11 display. (%#X)", status);
			break;

		case UIOHOOK_ERROR_X_RECORD_NOT_FOUND:
			logger_proc(LOG_LEVEL_ERROR, "Unable to locate XRecord extension. (%#X)", status);
			break;

		case UIOHOOK_ERROR_X_RECORD_ALLOC_RANGE:
			logger_proc(LOG_LEVEL_ERROR, "Unable to allocate XRecord range. (%#X)", status);
			break;

		case UIOHOOK_ERROR_X_RECORD_CREATE_CONTEXT:
			logger_proc(LOG_LEVEL_ERROR, "Unable to allocate XRecord context. (%#X)", status);
			break;

		case UIOHOOK_ERROR_X_RECORD_ENABLE_CONTEXT:
			logger_proc(LOG_LEVEL_ERROR, "Failed to enable XRecord context. (%#X)", status);
			break;

			
		// Windows specific errors.
		case UIOHOOK_ERROR_SET_WINDOWS_HOOK_EX:
			logger_proc(LOG_LEVEL_ERROR, "Failed to register low level windows hook. (%#X)", status);
			break;


		// Darwin specific errors.
		case UIOHOOK_ERROR_AXAPI_DISABLED:
			logger_proc(LOG_LEVEL_ERROR, "Failed to enable access for assistive devices. (%#X)", status);
			break;

		case UIOHOOK_ERROR_CREATE_EVENT_PORT:
			logger_proc(LOG_LEVEL_ERROR, "Failed to create apple event port. (%#X)", status);
			break;

		case UIOHOOK_ERROR_CREATE_RUN_LOOP_SOURCE:
			logger_proc(LOG_LEVEL_ERROR, "Failed to create apple run loop source. (%#X)", status);
			break;

		case UIOHOOK_ERROR_GET_RUNLOOP:
			logger_proc(LOG_LEVEL_ERROR, "Failed to acquire apple run loop. (%#X)", status);
			break;

		case UIOHOOK_ERROR_CREATE_OBSERVER:
			logger_proc(LOG_LEVEL_ERROR, "Failed to create apple run loop observer. (%#X)", status);
			break;

		// Default error.
		case UIOHOOK_FAILURE:
		default:
			logger_proc(LOG_LEVEL_ERROR, "An unknown hook error occurred. (%#X)", status);
			break;
	}

	return status;
}
