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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <uiohook.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef USE_XTEST
#include <X11/extensions/XTest.h>
#endif

#include "input_helper.h"
#include "logger.h"

#ifndef USE_XTEST
static long current_modifier_mask = NoEventMask;
#endif

static int post_key_event(uiohook_event * const event) {
    KeyCode keycode = scancode_to_keycode(event->data.keyboard.keycode);
    if (keycode == 0x0000) {
        logger(LOG_LEVEL_WARN, "%s [%u]: Unable to lookup scancode: %li\n",
                __FUNCTION__, __LINE__, event->data.keyboard.keycode);
        return UIOHOOK_FAILURE;
    }

    #ifdef USE_XTEST
    Bool is_pressed;
    #else
    long event_mask;

    XKeyEvent key_event = {
        .serial = 0x00,
        .time = CurrentTime,
        .same_screen = True,
        .send_event = False,
        .display = helper_disp,

        .root = XDefaultRootWindow(helper_disp),
        .window = None,
        .subwindow = None,

        .x_root = 0,
        .y_root = 0,
        .x = 0,
        .y = 0,

        .state = current_modifier_mask,
        .keycode = keycode
    };

    int revert;
    XGetInputFocus(helper_disp, &(key_event.window), &revert);
    #endif

    if (event->type == EVENT_KEY_PRESSED) {
        #ifdef USE_XTEST
        is_pressed = True;
        #else
        key_event.type = KeyPress;
        event_mask = KeyPressMask;

        switch (event->data.keyboard.keycode) {
            case VC_SHIFT_L:
            case VC_SHIFT_R:
                current_modifier_mask |= ShiftMask;
                break;

            case VC_CONTROL_L:
            case VC_CONTROL_R:
                current_modifier_mask |= ControlMask;
                break;

            case VC_META_L:
            case VC_META_R:
                current_modifier_mask |= Mod4Mask;
                break;

            case VC_ALT_L:
            case VC_ALT_R:
                current_modifier_mask |= Mod1Mask;
                break;
        }
        #endif

    } else if (event->type == EVENT_KEY_RELEASED) {
        #ifdef USE_XTEST
        is_pressed = False;
        #else
        key_event.type = KeyRelease;
        event_mask = KeyReleaseMask;

        switch (event->data.keyboard.keycode) {
            case VC_SHIFT_L:
            case VC_SHIFT_R:
                current_modifier_mask &= ~ShiftMask;
                break;

            case VC_CONTROL_L:
            case VC_CONTROL_R:
                current_modifier_mask &= ~ControlMask;
                break;

            case VC_META_L:
            case VC_META_R:
                current_modifier_mask &= ~Mod4Mask;
                break;

            case VC_ALT_L:
            case VC_ALT_R:
                current_modifier_mask &= ~Mod1Mask;
                break;
        }
        #endif
    } else {
        logger(LOG_LEVEL_DEBUG, "%s [%u]: Invalid event for keyboard post event: %#X.\n",
            __FUNCTION__, __LINE__, event->type);
        return UIOHOOK_FAILURE;
    }

    #ifdef USE_XTEST
    if (XTestFakeKeyEvent(helper_disp, keycode, is_pressed, 0) != Success) {
        logger(LOG_LEVEL_ERROR, "%s [%u]: XTestFakeKeyEvent() failed!\n",
            __FUNCTION__, __LINE__, event->type);
        return UIOHOOK_FAILURE;
    }
    #else
    XSelectInput(helper_disp, key_event.window, KeyPressMask | KeyReleaseMask);
    if (XSendEvent(helper_disp, key_event.window, False, event_mask, (XEvent *) &key_event) == 0) {
        logger(LOG_LEVEL_ERROR, "%s [%u]: XSendEvent() failed!\n",
            __FUNCTION__, __LINE__, event->type);
        return UIOHOOK_FAILURE;
    }
    #endif

    return UIOHOOK_SUCCESS;
}

static int post_mouse_button_event(uiohook_event * const event) {
    XButtonEvent btn_event = {
        .serial = 0,
        .send_event = False,
        .display = helper_disp,

        .window = None,                                   /* “event” window it is reported relative to */
        .root = None,                                     /* root window that the event occurred on */
        .subwindow = XDefaultRootWindow(helper_disp),     /* child window */

        .time = CurrentTime,

        .x = event->data.mouse.x,                         /* pointer x, y coordinates in event window */
        .y = event->data.mouse.y,

        .x_root = 0,                                      /* coordinates relative to root */
        .y_root = 0,

        .state = 0x00,                                    /* key or button mask */
        .same_screen = True
    };

    // Move the pointer to the specified position.
    #ifdef USE_XTEST
    XTestFakeMotionEvent(btn_event.display, -1, btn_event.x, btn_event.y, 0);
    #else
    XWarpPointer(btn_event.display, None, btn_event.subwindow, 0, 0, 0, 0, btn_event.x, btn_event.y);
    XFlush(btn_event.display);
    #endif

    #ifndef USE_XTEST
    // FIXME This is still not working correctly, clicking on other windows does not yield focus.
    while (btn_event.subwindow != None)
    {
        btn_event.window = btn_event.subwindow;
        XQueryPointer (
            btn_event.display,
            btn_event.window,
            &btn_event.root,
            &btn_event.subwindow,
            &btn_event.x_root,
            &btn_event.y_root,
            &btn_event.x,
            &btn_event.y,
            &btn_event.state
        );
    }
    #endif

    switch (event->type) {
        case EVENT_MOUSE_PRESSED:
            #ifdef USE_XTEST
            if (event->data.mouse.button < MOUSE_BUTTON1 || event->data.mouse.button > MOUSE_BUTTON5) {
                logger(LOG_LEVEL_WARN, "%s [%u]: Invalid button specified for mouse pressed event! (%u)\n",
                        __FUNCTION__, __LINE__, event->data.mouse.button);
                return UIOHOOK_FAILURE;
            }

            XTestFakeButtonEvent(helper_disp, event->data.mouse.button, True, 0);
            #else
            if (event->data.mouse.button == MOUSE_BUTTON1) {
                current_modifier_mask |= Button1MotionMask;
            } else if (event->data.mouse.button == MOUSE_BUTTON2) {
                current_modifier_mask |= Button2MotionMask;
            } else if (event->data.mouse.button == MOUSE_BUTTON3) {
                current_modifier_mask |= Button3MotionMask;
            } else if (event->data.mouse.button == MOUSE_BUTTON4) {
                current_modifier_mask |= Button4MotionMask;
            } else if (event->data.mouse.button == MOUSE_BUTTON5) {
                current_modifier_mask |= Button5MotionMask;
            } else {
                logger(LOG_LEVEL_WARN, "%s [%u]: Invalid button specified for mouse pressed event! (%u)\n",
                        __FUNCTION__, __LINE__, event->data.mouse.button);
                return UIOHOOK_FAILURE;
            }

            btn_event.type = ButtonPress;
            btn_event.button = event->data.mouse.button;
            btn_event.state = current_modifier_mask;
            XSendEvent(helper_disp, btn_event.window, False, ButtonPressMask, (XEvent *) &btn_event);
            #endif
            break;

        case EVENT_MOUSE_RELEASED:
            #ifdef USE_XTEST
            if (event->data.mouse.button < MOUSE_BUTTON1 || event->data.mouse.button > MOUSE_BUTTON5) {
                logger(LOG_LEVEL_WARN, "%s [%u]: Invalid button specified for mouse released event! (%u)\n",
                        __FUNCTION__, __LINE__, event->data.mouse.button);
                return UIOHOOK_FAILURE;
            }

            XTestFakeButtonEvent(helper_disp, event->data.mouse.button, False, 0);
            #else
            if (event->data.mouse.button == MOUSE_BUTTON1) {
                current_modifier_mask &= ~Button1MotionMask;
            } else if (event->data.mouse.button == MOUSE_BUTTON2) {
                current_modifier_mask &= ~Button2MotionMask;
            } else if (event->data.mouse.button == MOUSE_BUTTON3) {
                current_modifier_mask &= ~Button3MotionMask;
            } else if (event->data.mouse.button == MOUSE_BUTTON4) {
                current_modifier_mask &= ~Button4MotionMask;
            } else if (event->data.mouse.button == MOUSE_BUTTON5) {
                current_modifier_mask &= ~Button5MotionMask;
            } else {
                logger(LOG_LEVEL_WARN, "%s [%u]: Invalid button specified for mouse released event! (%u)\n",
                        __FUNCTION__, __LINE__, event->data.mouse.button);
                return UIOHOOK_FAILURE;
            }

            btn_event.type = ButtonRelease;
            btn_event.button = event->data.mouse.button;
            btn_event.state = current_modifier_mask;
            XSendEvent(helper_disp, btn_event.window, False, ButtonReleaseMask, (XEvent *) &btn_event);
            #endif
            break;

        default:
            logger(LOG_LEVEL_DEBUG, "%s [%u]: Invalid mouse button event: %#X.\n",
                __FUNCTION__, __LINE__, event->type);
            return UIOHOOK_FAILURE;
    }

    return UIOHOOK_SUCCESS;
}

static int post_mouse_wheel_event(uiohook_event * const event) {
    XButtonEvent btn_event = {
        .serial = 0,
        .send_event = False,
        .display = helper_disp,

        .window = None,                                   /* “event” window it is reported relative to */
        .root = None,                                     /* root window that the event occurred on */
        .subwindow = XDefaultRootWindow(helper_disp),     /* child window */

        .time = CurrentTime,

        .x = event->data.wheel.x,                         /* pointer x, y coordinates in event window */
        .y = event->data.wheel.y,

        .x_root = 0,                                      /* coordinates relative to root */
        .y_root = 0,

        .state = 0x00,                                    /* key or button mask */
        .same_screen = True
    };

    #ifndef USE_XTEST
    // FIXME This is still not working correctly, clicking on other windows does not yield focus.
    while (btn_event.subwindow != None)
    {
        btn_event.window = btn_event.subwindow;
        XQueryPointer (
            btn_event.display,
            btn_event.window,
            &btn_event.root,
            &btn_event.subwindow,
            &btn_event.x_root,
            &btn_event.y_root,
            &btn_event.x,
            &btn_event.y,
            &btn_event.state
        );
    }
    #endif

    // Wheel events should be the same as click events on X11.
    // type, amount and rotation
    unsigned int button = button_map_lookup(event->data.wheel.rotation < 0 ? WheelUp : WheelDown);

    #ifdef USE_XTEST
    XTestFakeButtonEvent(helper_disp, button, True, 0);
    #else
    btn_event.type = ButtonPress;
    btn_event.button = button;
    btn_event.state = current_modifier_mask;
    XSendEvent(helper_disp, btn_event.window, False, ButtonPressMask, (XEvent *) &btn_event);
    #endif

    #ifdef USE_XTEST
    XTestFakeButtonEvent(helper_disp, button, False, 0);
    #else
    btn_event.type = ButtonRelease;
    btn_event.button = button;
    btn_event.state = current_modifier_mask;
    XSendEvent(helper_disp, btn_event.window, False, ButtonReleaseMask, (XEvent *) &btn_event);
    #endif

    return UIOHOOK_SUCCESS;
}

static void post_mouse_motion_event(uiohook_event * const event) {
    #ifdef USE_XTEST
    XTestFakeMotionEvent(helper_disp, -1, event->data.mouse.x, event->data.mouse.y, 0);
    #else
    XMotionEvent mov_event = {
        .type = MotionNotify,
        .serial = 0,
        .send_event = False,
        .display = helper_disp,

        .window = None,                                   /* “event” window it is reported relative to */
        .root = XDefaultRootWindow(helper_disp),      /* root window that the event occurred on */
        .subwindow = None,                                /* child window */

        .time = CurrentTime,

        .x = event->data.mouse.x,                         /* pointer x, y coordinates in event window */
        .y = event->data.mouse.y,

        .x_root = event->data.mouse.x,                    /* coordinates relative to root */
        .y_root = event->data.mouse.x,

        .state = current_modifier_mask|MotionNotify,      /* key or button mask */

        .is_hint = NotifyNormal,
        .same_screen = True
    };

    int revert;
    XGetInputFocus(helper_disp, &(mov_event.window), &revert);

    XSendEvent(helper_disp, mov_event.window, False, mov_event.state, (XEvent *) &mov_event);
    #endif
}

// TODO This should return a status code, UIOHOOK_SUCCESS or otherwise.
UIOHOOK_API void hook_post_event(uiohook_event * const event) {
    if (helper_disp == NULL) {
        logger(LOG_LEVEL_ERROR, "%s [%u]: XDisplay helper_disp is unavailable!\n",
            __FUNCTION__, __LINE__);
        return; // UIOHOOK_ERROR_X_OPEN_DISPLAY
    }

    XLockDisplay(helper_disp);

    switch (event->type) {
        case EVENT_KEY_PRESSED:
        case EVENT_KEY_RELEASED:
            post_key_event(event);
            break;

        case EVENT_MOUSE_PRESSED:
        case EVENT_MOUSE_RELEASED:
            post_mouse_button_event(event);
            break;

        case EVENT_MOUSE_WHEEL:
            post_mouse_wheel_event(event);
            break;

        case EVENT_MOUSE_MOVED:
        case EVENT_MOUSE_DRAGGED:
            post_mouse_motion_event(event);
            break;

        case EVENT_KEY_TYPED:
        case EVENT_MOUSE_CLICKED:

        case EVENT_HOOK_ENABLED:
        case EVENT_HOOK_DISABLED:

        default:
            logger(LOG_LEVEL_WARN, "%s [%u]: Ignoring post event type %#X\n",
                __FUNCTION__, __LINE__, event->type);
            break;
    }

    // Don't forget to flush!
    XSync(helper_disp, True);
    XUnlockDisplay(helper_disp);
}
