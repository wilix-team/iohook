/* libUIOHook: Cross-platform keyboard and mouse hooking from userland.
 * Copyright (C) 2006-2022 Alexander Barker.  All Rights Reserved.
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
#include <uiohook.h>
#include <windows.h>

#include "input_helper.h"
#include "logger.h"

// Some buggy versions of MinGW and MSys do not include these constants in winuser.h.
#ifndef MAPVK_VK_TO_VSC
#define MAPVK_VK_TO_VSC         0
#define MAPVK_VSC_TO_VK         1
#define MAPVK_VK_TO_CHAR        2
#define MAPVK_VSC_TO_VK_EX      3
#endif
// Some buggy versions of MinGW and MSys only define this value for Windows
// versions >= 0x0600 (Windows Vista) when it should be 0x0500 (Windows 2000).
#ifndef MAPVK_VK_TO_VSC_EX
#define MAPVK_VK_TO_VSC_EX      4
#endif

#ifndef KEYEVENTF_SCANCODE
#define KEYEVENTF_EXTENDEDKEY   0x0001
#define KEYEVENTF_KEYUP         0x0002
#define KEYEVENTF_UNICODE       0x0004
#define KEYEVENTF_SCANCODE      0x0008
#endif

#ifndef KEYEVENTF_KEYDOWN
#define KEYEVENTF_KEYDOWN       0x0000
#endif

#define MAX_WINDOWS_COORD_VALUE (1 << 16)

// TODO I doubt this table is complete.
// http://letcoderock.blogspot.fr/2011/10/sendinput-with-shift-key-not-work.html
static const uint16_t extend_key_table[10] = {
    VK_UP,
    VK_DOWN,
    VK_LEFT,
    VK_RIGHT,
    VK_HOME,
    VK_END,
    VK_PRIOR, // PgUp
    VK_NEXT,  //  PgDn
    VK_INSERT,
    VK_DELETE
};


static LONG convert_to_relative_position(int coordinate, int screen_size) {
    // See https://stackoverflow.com/a/4555214 and its comments
	int offset = (coordinate > 0 ? 1 : -1); // Negative coordinates appear when using multiple monitors
	return ((coordinate * MAX_WINDOWS_COORD_VALUE) / screen_size) + offset;
}

static int map_keyboard_event(uiohook_event * const event, INPUT * const input) {
    input->type = INPUT_KEYBOARD; // | KEYEVENTF_SCANCODE
    //input->ki.wScan = event->data.keyboard.rawcode;
    //input->ki.time = GetSystemTime();

    switch (event->type) {
        case EVENT_KEY_PRESSED:
            input->ki.dwFlags = KEYEVENTF_KEYDOWN;
            break;

        case EVENT_KEY_RELEASED:
            input->ki.dwFlags = KEYEVENTF_KEYUP;
            break;

        default:
            logger(LOG_LEVEL_DEBUG, "%s [%u]: Invalid event for keyboard event mapping: %#X.\n",
                __FUNCTION__, __LINE__, event->type);
            return UIOHOOK_FAILURE;
    }

    input->ki.wVk = (WORD) scancode_to_keycode(event->data.keyboard.keycode);
    if (input->ki.wVk == 0x0000) {
        logger(LOG_LEVEL_WARN, "%s [%u]: Unable to lookup scancode: %li\n",
                __FUNCTION__, __LINE__, event->data.keyboard.keycode);
        return UIOHOOK_FAILURE;
    }

    // FIXME Why is this checking MASK_SHIFT
    if (event->mask & MASK_SHIFT) {
        for (int i = 0; i < sizeof(extend_key_table) / sizeof(uint16_t)
                && input->ki.wVk == extend_key_table[i]; i++) {
            input->ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        }
    }

    return UIOHOOK_SUCCESS;
}

static int map_mouse_event(uiohook_event * const event, INPUT * const input) {
    // FIXME implement multiple monitor support
    uint16_t screen_width  = GetSystemMetrics(SM_CXSCREEN);
    uint16_t screen_height = GetSystemMetrics(SM_CYSCREEN);

    input->type = INPUT_MOUSE;
    input->mi.mouseData = 0;
    input->mi.dwExtraInfo = 0;
    input->mi.time = 0; // GetSystemTime();

    input->mi.dx = convert_to_relative_position(event->data.mouse.x, screen_width);
    input->mi.dy = convert_to_relative_position(event->data.mouse.y, screen_height);

    switch (event->type) {
        case EVENT_MOUSE_PRESSED:
            if (event->data.mouse.button == MOUSE_NOBUTTON) {
                logger(LOG_LEVEL_WARN, "%s [%u]: No button specified for mouse pressed event!\n",
                        __FUNCTION__, __LINE__);
                return UIOHOOK_FAILURE;
            } else if (event->data.mouse.button == MOUSE_BUTTON1) {
                input->mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            } else if (event->data.mouse.button == MOUSE_BUTTON2) {
                input->mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            } else if (event->data.mouse.button == MOUSE_BUTTON3) {
                input->mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
            } else {
                input->mi.dwFlags = MOUSEEVENTF_XDOWN;
                if (event->data.mouse.button == MOUSE_BUTTON4) {
                    input->mi.mouseData = XBUTTON1;
                } else if (event->data.mouse.button == MOUSE_BUTTON5) {
                    input->mi.mouseData = XBUTTON2;
                } else {
                    input->mi.mouseData = event->data.mouse.button - 3;
                }
            }

            // We need to move the mouse to the correct location prior to clicking.
            event->type = EVENT_MOUSE_MOVED;
            // TODO Remember to check the status here.
            hook_post_event(event);
            event->type = EVENT_MOUSE_PRESSED;
            break;

        case EVENT_MOUSE_RELEASED:
            if (event->data.mouse.button == MOUSE_NOBUTTON) {
                logger(LOG_LEVEL_WARN, "%s [%u]: No button specified for mouse released event!\n",
                        __FUNCTION__, __LINE__);
                return UIOHOOK_FAILURE;
            } else if (event->data.mouse.button == MOUSE_BUTTON1) {
                input->mi.dwFlags = MOUSEEVENTF_LEFTUP;
            } else if (event->data.mouse.button == MOUSE_BUTTON2) {
                input->mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            } else if (event->data.mouse.button == MOUSE_BUTTON3) {
                input->mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
            } else {
                input->mi.dwFlags = MOUSEEVENTF_XUP;
                if (event->data.mouse.button == MOUSE_BUTTON4) {
                    input->mi.mouseData = XBUTTON1;
                } else if (event->data.mouse.button == MOUSE_BUTTON5) {
                    input->mi.mouseData = XBUTTON2;
                } else {
                    input->mi.mouseData = event->data.mouse.button - 3;
                }
            }

            // We need to move the mouse to the correct location prior to clicking.
            event->type = EVENT_MOUSE_MOVED;
            // TODO Remember to check the status here.
            hook_post_event(event);
            event->type = EVENT_MOUSE_PRESSED;
            break;

        case EVENT_MOUSE_WHEEL:
            input->mi.dwFlags = MOUSEEVENTF_WHEEL;

            // type, amount and rotation?
            input->mi.mouseData = event->data.wheel.amount * event->data.wheel.rotation * WHEEL_DELTA;
            break;

        case EVENT_MOUSE_DRAGGED:
        case EVENT_MOUSE_MOVED:
            input->mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK;
            break;

        default:
            logger(LOG_LEVEL_DEBUG, "%s [%u]: Invalid event for mouse event mapping: %#X.\n",
                __FUNCTION__, __LINE__, event->type);
            return UIOHOOK_FAILURE;
    }

    return UIOHOOK_SUCCESS;
}

// TODO This should return a status code, UIOHOOK_SUCCESS or otherwise.
UIOHOOK_API void hook_post_event(uiohook_event * const event) {
    int status = UIOHOOK_FAILURE;

    INPUT *input = (INPUT *) calloc(1, sizeof(INPUT))   ;
    if (input == NULL) {
        logger(LOG_LEVEL_ERROR, "%s [%u]: failed to allocate memory: calloc!\n",
                __FUNCTION__, __LINE__);
        return; // UIOHOOK_ERROR_OUT_OF_MEMORY
    }

    switch (event->type) {
        case EVENT_KEY_PRESSED:
        case EVENT_KEY_RELEASED:
            status = map_keyboard_event(event, input);
            break;

        case EVENT_MOUSE_PRESSED:
        case EVENT_MOUSE_RELEASED:
        case EVENT_MOUSE_WHEEL:
        case EVENT_MOUSE_MOVED:
        case EVENT_MOUSE_DRAGGED:
            status = map_mouse_event(event, input);
            break;

        case EVENT_KEY_TYPED:
        case EVENT_MOUSE_CLICKED:

        case EVENT_HOOK_ENABLED:
        case EVENT_HOOK_DISABLED:

        default:
            logger(LOG_LEVEL_DEBUG, "%s [%u]: Ignoring post event: %#X.\n",
                __FUNCTION__, __LINE__, event->type);

    }

    if (status != UIOHOOK_FAILURE && !SendInput(1, input, sizeof(INPUT))) {
        logger(LOG_LEVEL_ERROR, "%s [%u]: SendInput() failed! (%#lX)\n",
                __FUNCTION__, __LINE__, (unsigned long) GetLastError());
    }

    free(input);
}
