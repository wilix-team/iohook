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

#include <dlfcn.h>
#include <mach/mach_time.h>

#ifdef USE_OBJC
#include <objc/objc.h>
#include <objc/objc-runtime.h>
#endif

#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>
#include <uiohook.h>

#include "input_helper.h"
#include "logger.h"


typedef struct _event_runloop_info {
    CFMachPortRef port;
    CFRunLoopSourceRef source;
    CFRunLoopObserverRef observer;
} event_runloop_info;

#ifdef USE_OBJC
static id auto_release_pool;

typedef struct {
    CGEventRef event;
    UInt32 subtype;
    UInt32 data1;
} TISEventMessage;
TISEventMessage *tis_event_message;
#endif

// Event runloop reference.
CFRunLoopRef event_loop;

// Flag to restart the event tap incase of timeout.
static Boolean restart_tap = false;

// Modifiers for tracking key masks.
static uint16_t current_modifiers = 0x0000;

// Required to transport messages between the main runloop and our thread for
// Unicode lookups.
#define KEY_BUFFER_SIZE 4
typedef struct {
    CGEventRef event;
    UniChar buffer[KEY_BUFFER_SIZE];
    UniCharCount length;
} TISKeycodeMessage;
TISKeycodeMessage *tis_keycode_message;

#if __MAC_OS_X_VERSION_MAX_ALLOWED <= 1050
typedef void* dispatch_queue_t;
#endif
static struct dispatch_queue_s *dispatch_main_queue_s;
static void (*dispatch_sync_f_f)(dispatch_queue_t, void *, void (*function)(void *));

#if !defined(USE_CARBON_LEGACY) && defined(USE_APPLICATION_SERVICES)
typedef struct _main_runloop_info {
    CFRunLoopSourceRef source;
    CFRunLoopObserverRef observer;
} main_runloop_info;

main_runloop_info *main_runloop_keycode = NULL;

static pthread_cond_t main_runloop_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t main_runloop_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

// Click count globals.
static unsigned short click_count = 0;
static CGEventTimestamp click_time = 0;
static unsigned short int click_button = MOUSE_NOBUTTON;
static bool mouse_dragged = false;

// Structure for the current Unix epoch in milliseconds.
static struct timeval system_time;

// Virtual event pointer.
static uiohook_event event;

// Event dispatch callback.
static dispatcher_t dispatcher = NULL;

UIOHOOK_API void hook_set_dispatch_proc(dispatcher_t dispatch_proc) {
    logger(LOG_LEVEL_DEBUG, "%s [%u]: Setting new dispatch callback to %#p.\n",
            __FUNCTION__, __LINE__, dispatch_proc);

    dispatcher = dispatch_proc;
}

// Send out an event if a dispatcher was set.
static inline void dispatch_event(uiohook_event *const event) {
    if (dispatcher != NULL) {
        logger(LOG_LEVEL_DEBUG, "%s [%u]: Dispatching event type %u.\n",
                __FUNCTION__, __LINE__, event->type);

        dispatcher(event);
    } else {
        logger(LOG_LEVEL_WARN, "%s [%u]: No dispatch callback set!\n",
                __FUNCTION__, __LINE__);
    }
}


// Set the native modifier mask for future events.
static inline void set_modifier_mask(uint16_t mask) {
    current_modifiers |= mask;
}

// Unset the native modifier mask for future events.
static inline void unset_modifier_mask(uint16_t mask) {
    current_modifiers &= ~mask;
}

// Get the current native modifier mask state.
static inline uint16_t get_modifiers() {
    return current_modifiers;
}

// Initialize the modifier mask to the current modifiers.
static void initialize_modifiers() {
    current_modifiers = 0x0000;

    if (CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, kVK_Shift)) {
        set_modifier_mask(MASK_SHIFT_L);
    }
    if (CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, kVK_RightShift)) {
        set_modifier_mask(MASK_SHIFT_R);
    }

    if (CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, kVK_Control)) {
        set_modifier_mask(MASK_CTRL_L);
    }
    if (CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, kVK_RightControl)) {
        set_modifier_mask(MASK_CTRL_R);
    }

    if (CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, kVK_Option)) {
        set_modifier_mask(MASK_ALT_L);
    }
    if (CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, kVK_RightOption)) {
        set_modifier_mask(MASK_ALT_R);
    }

    if (CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, kVK_Command)) {
        set_modifier_mask(MASK_META_L);
    }
    if (CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, kVK_RightCommand)) {
        set_modifier_mask(MASK_META_R);
    }

    if (CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kVK_LBUTTON)) {
        set_modifier_mask(MASK_BUTTON1);
    }
    if (CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kVK_RBUTTON)) {
        set_modifier_mask(MASK_BUTTON2);
    }

    if (CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kVK_MBUTTON)) {
        set_modifier_mask(MASK_BUTTON3);
    }
    if (CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kVK_XBUTTON1)) {
        set_modifier_mask(MASK_BUTTON4);
    }
    if (CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kVK_XBUTTON2)) {
        set_modifier_mask(MASK_BUTTON5);
    }

    if (CGEventSourceFlagsState(kCGEventSourceStateCombinedSessionState) & kCGEventFlagMaskAlphaShift) {
        set_modifier_mask(MASK_CAPS_LOCK);
    }
    // Best I can tell, OS X does not support Num or Scroll lock.
    unset_modifier_mask(MASK_NUM_LOCK);
    unset_modifier_mask(MASK_SCROLL_LOCK);
}


// Wrap keycode_to_unicode with some null checks.
static void keycode_to_lookup(void *info) {
    TISKeycodeMessage *data = (TISKeycodeMessage *) info;

    if (data != NULL && data->event != NULL) {
        // Preform Unicode lookup.
        data->length = keycode_to_unicode(data->event, data->buffer, KEY_BUFFER_SIZE);
    }
}

static void hook_status_proc(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *info) {
    uint64_t timestamp = mach_absolute_time();

    switch (activity) {
        case kCFRunLoopEntry:
            // Initialize Native Input Functions.
            load_input_helper();

            // Populate the hook start event.
            event.time = timestamp;
            event.reserved = 0x00;

            event.type = EVENT_HOOK_ENABLED;
            event.mask = 0x00;

            // Fire the hook start event.
            dispatch_event(&event);
            break;

        case kCFRunLoopExit:
            // Populate the hook stop event.
            event.time = timestamp;
            event.reserved = 0x00;

            event.type = EVENT_HOOK_DISABLED;
            event.mask = 0x00;

            // Fire the hook stop event.
            dispatch_event(&event);
            
            // Deinitialize native input helper functions.
            unload_input_helper();
            break;

        default:
            logger(LOG_LEVEL_WARN, "%s [%u]: Unhandled RunLoop activity! (%#X)\n",
                    __FUNCTION__, __LINE__, (unsigned int) activity);
            break;
    }
}

static inline void process_key_pressed(uint64_t timestamp, CGEventRef event_ref) {
    UInt64 keycode = CGEventGetIntegerValueField(event_ref, kCGKeyboardEventKeycode);

    // Populate key pressed event.
    event.time = timestamp;
    event.reserved = 0x00;

    event.type = EVENT_KEY_PRESSED;
    event.mask = get_modifiers();

    event.data.keyboard.keycode = keycode_to_scancode(keycode);
    event.data.keyboard.rawcode = keycode;
    event.data.keyboard.keychar = CHAR_UNDEFINED;

    logger(LOG_LEVEL_DEBUG, "%s [%u]: Key %#X pressed. (%#X)\n",
            __FUNCTION__, __LINE__, event.data.keyboard.keycode, event.data.keyboard.rawcode);

    // Fire key pressed event.
    dispatch_event(&event);

    // If the pressed event was not consumed...
    if (event.reserved ^ 0x01) {
        tis_keycode_message->event = event_ref;
        tis_keycode_message->length = 0;
        bool is_runloop_main = CFEqual(event_loop, CFRunLoopGetMain());

        if (dispatch_sync_f_f != NULL && dispatch_main_queue_s != NULL && !is_runloop_main) {
            logger(LOG_LEVEL_DEBUG, "%s [%u]: Using dispatch_sync_f for key typed events.\n",
                    __FUNCTION__, __LINE__);
            (*dispatch_sync_f_f)(dispatch_main_queue_s, tis_keycode_message, &keycode_to_lookup);
        }
        #if !defined(USE_CARBON_LEGACY) && defined(USE_APPLICATION_SERVICES)
        else if (!is_runloop_main) {
            logger(LOG_LEVEL_DEBUG, "%s [%u]: Using CFRunLoopWakeUp for key typed events.\n",
                    __FUNCTION__, __LINE__);

            // Lock for code dealing with the main runloop.
            pthread_mutex_lock(&main_runloop_mutex);

            // Check to see if the main runloop is still running.
            // TODO I would rather this be a check on hook_enable(),
            // but it makes the usage complicated by requiring a separate
            // thread for the main runloop and hook registration.
            CFStringRef mode = CFRunLoopCopyCurrentMode(CFRunLoopGetMain());
            if (mode != NULL) {
                CFRelease(mode);

                // Lookup the Unicode representation for this event.
                //CFRunLoopSourceContext context = { .version = 0 };
                //CFRunLoopSourceGetContext(main_runloop_keycode->source, &context);

                // Get the run loop context info pointer.
                //TISKeycodeMessage *info = (TISKeycodeMessage *) context.info;

                // Set the event pointer.
                //info->event = event_ref;


                // Signal the custom source and wakeup the main runloop.
                CFRunLoopSourceSignal(main_runloop_keycode->source);
                CFRunLoopWakeUp(CFRunLoopGetMain());

                // Wait for a lock while the main runloop processes they key typed event.
                pthread_cond_wait(&main_runloop_cond, &main_runloop_mutex);
            }
            else {
                logger(LOG_LEVEL_WARN, "%s [%u]: Failed to signal RunLoop main!\n",
                        __FUNCTION__, __LINE__);
            }

            // Unlock for code dealing with the main runloop.
            pthread_mutex_unlock(&main_runloop_mutex);
        }
        #endif
        else {
            keycode_to_lookup(tis_keycode_message);
        }

        for (unsigned int i = 0; i < tis_keycode_message->length; i++) {
            // Populate key typed event.
            event.time = timestamp;
            event.reserved = 0x00;

            event.type = EVENT_KEY_TYPED;
            event.mask = get_modifiers();

            event.data.keyboard.keycode = VC_UNDEFINED;
            event.data.keyboard.rawcode = keycode;
            event.data.keyboard.keychar = tis_keycode_message->buffer[i];

            logger(LOG_LEVEL_DEBUG, "%s [%u]: Key %#X typed. (%lc)\n",
                    __FUNCTION__, __LINE__, event.data.keyboard.keycode,
                    (wint_t) event.data.keyboard.keychar);

            // Populate key typed event.
            dispatch_event(&event);
        }
    }
}

static inline void process_key_released(uint64_t timestamp, CGEventRef event_ref) {
    UInt64 keycode = CGEventGetIntegerValueField(event_ref, kCGKeyboardEventKeycode);

    // Populate key released event.
    event.time = timestamp;
    event.reserved = 0x00;

    event.type = EVENT_KEY_RELEASED;
    event.mask = get_modifiers();

    event.data.keyboard.keycode = keycode_to_scancode(keycode);
    event.data.keyboard.rawcode = keycode;
    event.data.keyboard.keychar = CHAR_UNDEFINED;

    logger(LOG_LEVEL_DEBUG, "%s [%u]: Key %#X released. (%#X)\n",
            __FUNCTION__, __LINE__, event.data.keyboard.keycode, event.data.keyboard.rawcode);

    // Fire key released event.
    dispatch_event(&event);
}

static inline void process_modifier_changed(uint64_t timestamp, CGEventRef event_ref) {
    CGEventFlags event_mask = CGEventGetFlags(event_ref);
    UInt64 keycode = CGEventGetIntegerValueField(event_ref, kCGKeyboardEventKeycode);

    logger(LOG_LEVEL_DEBUG, "%s [%u]: Modifiers Changed for key %#X. (%#X)\n",
            __FUNCTION__, __LINE__, (unsigned long) keycode, (unsigned int) event_mask);

    /* Because Apple treats modifier keys differently than normal key
     * events, any changes to the modifier keys will require a key state
     * change to be fired manually.
     *
     * NOTE Left and right keyboard masks like NX_NEXTLSHIFTKEYMASK exist and
     * appear to be in use on Darwin, however they are removed by comment or
     * preprocessor with a note that reads "device-dependent (really?)."  To
     * ensure compatability, we will do this the verbose way.
     *
     * NOTE The masks for scroll and number lock are set in the key event.
     */
    if (keycode == kVK_Shift) {
        if (event_mask & kCGEventFlagMaskShift) {
            // Process as a key pressed event.
            set_modifier_mask(MASK_SHIFT_L);
            process_key_pressed(timestamp, event_ref);
        } else {
            // Process as a key released event.
            unset_modifier_mask(MASK_SHIFT_L);
            process_key_released(timestamp, event_ref);
        }
    } else if (keycode == kVK_Control) {
        if (event_mask & kCGEventFlagMaskControl) {
            // Process as a key pressed event.
            set_modifier_mask(MASK_CTRL_L);
            process_key_pressed(timestamp, event_ref);
        } else {
            // Process as a key released event.
            unset_modifier_mask(MASK_CTRL_L);
            process_key_released(timestamp, event_ref);
        }
    } else if (keycode == kVK_Command) {
        if (event_mask & kCGEventFlagMaskCommand) {
            // Process as a key pressed event.
            set_modifier_mask(MASK_META_L);
            process_key_pressed(timestamp, event_ref);
        } else {
            // Process as a key released event.
            unset_modifier_mask(MASK_META_L);
            process_key_released(timestamp, event_ref);
        }
    } else if (keycode == kVK_Option) {
        if (event_mask & kCGEventFlagMaskAlternate) {
            // Process as a key pressed event.
            set_modifier_mask(MASK_ALT_L);
            process_key_pressed(timestamp, event_ref);
        } else {
            // Process as a key released event.
            unset_modifier_mask(MASK_ALT_L);
            process_key_released(timestamp, event_ref);
        }
    } else if (keycode == kVK_RightShift) {
        if (event_mask & kCGEventFlagMaskShift) {
            // Process as a key pressed event.
            set_modifier_mask(MASK_SHIFT_R);
            process_key_pressed(timestamp, event_ref);
        } else {
            // Process as a key released event.
            unset_modifier_mask(MASK_SHIFT_R);
            process_key_released(timestamp, event_ref);
        }
    } else if (keycode == kVK_RightControl) {
        if (event_mask & kCGEventFlagMaskControl) {
            // Process as a key pressed event.
            set_modifier_mask(MASK_CTRL_R);
            process_key_pressed(timestamp, event_ref);
        } else {
            // Process as a key released event.
            unset_modifier_mask(MASK_CTRL_R);
            process_key_released(timestamp, event_ref);
        }
    } else if (keycode == kVK_RightCommand) {
        if (event_mask & kCGEventFlagMaskCommand) {
            // Process as a key pressed event.
            set_modifier_mask(MASK_META_R);
            process_key_pressed(timestamp, event_ref);
        } else {
            // Process as a key released event.
            unset_modifier_mask(MASK_META_R);
            process_key_released(timestamp, event_ref);
        }
    } else if (keycode == kVK_RightOption) {
        if (event_mask & kCGEventFlagMaskAlternate) {
            // Process as a key pressed event.
            set_modifier_mask(MASK_ALT_R);
            process_key_pressed(timestamp, event_ref);
        } else {
            // Process as a key released event.
            unset_modifier_mask(MASK_ALT_R);
            process_key_released(timestamp, event_ref);
        }
    } else if (keycode == kVK_CapsLock) {
        if (current_modifiers & MASK_CAPS_LOCK) {
            // Process as a key pressed event.
            unset_modifier_mask(MASK_CAPS_LOCK);
            // Key released handled by process_system_key
        } else {
            // Process as a key released event.
            set_modifier_mask(MASK_CAPS_LOCK);
            // Key pressed handled by process_system_key
        }
    }
}

#ifdef USE_OBJC
static void obcj_message(void *info) {
    TISEventMessage *data = (TISEventMessage *) info;

    if (data != NULL && data->event != NULL) {
        // Contributed by Iván Munsuri Ibáñez <munsuri@gmail.com> and Alex <universailp@web.de>
        id (*eventWithCGEvent)(id, SEL, CGEventRef) = (id (*)(id, SEL, CGEventRef)) objc_msgSend;
        id event_data = eventWithCGEvent((id) objc_getClass("NSEvent"), sel_registerName("eventWithCGEvent:"), data->event);

        UInt32 (*eventWithoutCGEvent)(id, SEL) = (UInt32 (*)(id, SEL)) objc_msgSend;
        data->subtype = eventWithoutCGEvent(event_data, sel_registerName("subtype"));
        data->data1 = eventWithoutCGEvent(event_data, sel_registerName("data1"));
    }
}
#endif

/* These events are totally undocumented for the CGEvent type, but are required to grab media and caps-lock keys.
 */
static inline void process_system_key(uint64_t timestamp, CGEventRef event_ref) {
    if (CGEventGetType(event_ref) == NX_SYSDEFINED) {
        UInt32 subtype = 0;
        UInt32 data1 = 0;

        #ifdef USE_OBJC
        bool is_runloop_main = CFEqual(event_loop, CFRunLoopGetMain());
        tis_event_message->event = event_ref;
        tis_event_message->subtype = 0;
        tis_event_message->data1 = 0;

        if (dispatch_sync_f_f != NULL && dispatch_main_queue_s != NULL && !is_runloop_main) {
            logger(LOG_LEVEL_DEBUG, "%s [%u]: Using dispatch_sync_f for system key events.\n",
                    __FUNCTION__, __LINE__);

            (*dispatch_sync_f_f)(dispatch_main_queue_s, tis_event_message, &obcj_message);
            subtype = tis_event_message->subtype;
            data1 = tis_event_message->data1;
        } else if (is_runloop_main) {
            obcj_message(tis_event_message);
            subtype = tis_event_message->subtype;
            data1 = tis_event_message->data1;
        } else {
        #endif
        // If we are not using ObjC, the only way I've found to access CGEvent->subtype and CGEvent>data1 is to
        // serialize the event and read the byte offsets.  I am not sure why, but CGEventCreateData appears to use
        // big-endian byte ordering even though all current apple architectures are little-endian.
        CFDataRef data_ref = CGEventCreateData(kCFAllocatorDefault, event_ref);
        if (data_ref == NULL) {
            logger(LOG_LEVEL_ERROR, "%s [%u]: Failed to allocate memory for CGEventRef copy!\n",
                    __FUNCTION__, __LINE__);
            return;
        }

        if (CFDataGetLength(data_ref) < 132)
        {
            CFRelease(data_ref);
            logger(LOG_LEVEL_ERROR, "%s [%u]: Insufficient CFData range size!\n",
                    __FUNCTION__, __LINE__);
            return;
        }

        UInt8 *buffer = malloc(4);
        if (buffer == NULL) {
            CFRelease(data_ref);
            logger(LOG_LEVEL_ERROR, "%s [%u]: Failed to allocate memory for CFData range buffer!\n",
                    __FUNCTION__, __LINE__);
            return;
        }

        CFDataGetBytes(data_ref, CFRangeMake(120, 4), buffer);
        subtype = CFSwapInt32BigToHost(*((UInt32 *) buffer));

        CFDataGetBytes(data_ref, CFRangeMake(128, 4), buffer);
        data1 = CFSwapInt32BigToHost(*((UInt32 *) buffer));

        free(buffer);
        CFRelease(data_ref);
        #ifdef USE_OBJC
        }
        #endif

        if (subtype == 8) {
            int key_code = (data1 & 0xFFFF0000) >> 16;
            int key_flags = (data1 & 0xFFFF);
            int key_state = (key_flags & 0xFF00) >> 8;
            bool key_down = (key_state & 0x1) == 0;

            if (key_code == NX_KEYTYPE_CAPS_LOCK) {
                // It doesn't appear like we can modify the event coming in, so we will fabricate a new event.
                CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
                CGEventRef ns_event = CGEventCreateKeyboardEvent(src, kVK_CapsLock, key_down);
                CGEventSetFlags(ns_event, CGEventGetFlags(event_ref));

                if (key_down) {
                    process_key_pressed(timestamp, ns_event);
                } else {
                    process_key_released(timestamp, ns_event);
                }

                CFRelease(ns_event);
                CFRelease(src);
            } else if (key_code == NX_KEYTYPE_SOUND_UP) {
                // It doesn't appear like we can modify the event coming in, so we will fabricate a new event.
                CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
                CGEventRef ns_event = CGEventCreateKeyboardEvent(src, kVK_VolumeUp, key_down);
                CGEventSetFlags(ns_event, CGEventGetFlags(event_ref));

                if (key_down) {
                    process_key_pressed(timestamp, ns_event);
                } else {
                    process_key_released(timestamp, ns_event);
                }

                CFRelease(ns_event);
                CFRelease(src);
            } else if (key_code == NX_KEYTYPE_SOUND_DOWN) {
                // It doesn't appear like we can modify the event coming in, so we will fabricate a new event.
                CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
                CGEventRef ns_event = CGEventCreateKeyboardEvent(src, kVK_VolumeDown, key_down);
                CGEventSetFlags(ns_event, CGEventGetFlags(event_ref));

                if (key_down) {
                    process_key_pressed(timestamp, ns_event);
                } else {
                    process_key_released(timestamp, ns_event);
                }

                CFRelease(ns_event);
                CFRelease(src);
            } else if (key_code == NX_KEYTYPE_MUTE) {
                // It doesn't appear like we can modify the event coming in, so we will fabricate a new event.
                CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
                CGEventRef ns_event = CGEventCreateKeyboardEvent(src, kVK_Mute, key_down);
                CGEventSetFlags(ns_event, CGEventGetFlags(event_ref));

                if (key_down) {
                    process_key_pressed(timestamp, ns_event);
                } else {
                    process_key_released(timestamp, ns_event);
                }

                CFRelease(ns_event);
                CFRelease(src);
            } else if (key_code == NX_KEYTYPE_EJECT) {
                // It doesn't appear like we can modify the event coming in, so we will fabricate a new event.
                CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
                CGEventRef ns_event = CGEventCreateKeyboardEvent(src, kVK_NX_Eject, key_down);
                CGEventSetFlags(ns_event, CGEventGetFlags(event_ref));

                if (key_down) {
                    process_key_pressed(timestamp, ns_event);
                } else {
                    process_key_released(timestamp, ns_event);
                }

                CFRelease(ns_event);
                CFRelease(src);
            } else if (key_code == NX_KEYTYPE_PLAY) {
                // It doesn't appear like we can modify the event coming in, so we will fabricate a new event.
                CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
                CGEventRef ns_event = CGEventCreateKeyboardEvent(src, kVK_MEDIA_Play, key_down);
                CGEventSetFlags(ns_event, CGEventGetFlags(event_ref));

                if (key_down) {
                    process_key_pressed(timestamp, ns_event);
                } else {
                    process_key_released(timestamp, ns_event);
                }

                CFRelease(ns_event);
                CFRelease(src);
            } else if (key_code == NX_KEYTYPE_FAST) {
                // It doesn't appear like we can modify the event coming in, so we will fabricate a new event.
                CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
                CGEventRef ns_event = CGEventCreateKeyboardEvent(src, kVK_MEDIA_Next, key_down);
                CGEventSetFlags(ns_event, CGEventGetFlags(event_ref));

                if (key_down) {
                    process_key_pressed(timestamp, ns_event);
                } else {
                    process_key_released(timestamp, ns_event);
                }

                CFRelease(ns_event);
                CFRelease(src);
            } else if (key_code == NX_KEYTYPE_REWIND) {
                // It doesn't appear like we can modify the event coming in, so we will fabricate a new event.
                CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
                CGEventRef ns_event = CGEventCreateKeyboardEvent(src, kVK_MEDIA_Previous, key_down);
                CGEventSetFlags(ns_event, CGEventGetFlags(event_ref));

                if (key_down) {
                    process_key_pressed(timestamp, ns_event);
                } else {
                    process_key_released(timestamp, ns_event);
                }

                CFRelease(ns_event);
                CFRelease(src);
            }
        }
    }
}


static inline void process_button_pressed(uint64_t timestamp, CGEventRef event_ref, uint16_t button) {
    // Track the number of clicks.
    if (button == click_button && (long int) (timestamp - click_time) / 1000000 <= hook_get_multi_click_time()) {
        if (click_count < USHRT_MAX) {
            click_count++;
        }
        else {
            logger(LOG_LEVEL_WARN, "%s [%u]: Click count overflow detected!\n",
                    __FUNCTION__, __LINE__);
        }
    } else {
        // Reset the click count.
        click_count = 1;

        // Set the previous button.
        click_button = button;
    }

    // Save this events time to calculate the click_count.
    click_time = timestamp;

    CGPoint event_point = CGEventGetLocation(event_ref);

    // Populate mouse pressed event.
    event.time = timestamp;
    event.reserved = 0x00;

    event.type = EVENT_MOUSE_PRESSED;
    event.mask = get_modifiers();

    event.data.mouse.button = button;
    event.data.mouse.clicks = click_count;
    event.data.mouse.x = event_point.x;
    event.data.mouse.y = event_point.y;

    logger(LOG_LEVEL_DEBUG, "%s [%u]: Button %u pressed %u time(s). (%u, %u)\n",
            __FUNCTION__, __LINE__, event.data.mouse.button, event.data.mouse.clicks,
            event.data.mouse.x, event.data.mouse.y);

    // Fire mouse pressed event.
    dispatch_event(&event);
}

static inline void process_button_released(uint64_t timestamp, CGEventRef event_ref, uint16_t button) {
    CGPoint event_point = CGEventGetLocation(event_ref);

    // Populate mouse released event.
    event.time = timestamp;
    event.reserved = 0x00;

    event.type = EVENT_MOUSE_RELEASED;
    event.mask = get_modifiers();

    event.data.mouse.button = button;
    event.data.mouse.clicks = click_count;
    event.data.mouse.x = event_point.x;
    event.data.mouse.y = event_point.y;

    logger(LOG_LEVEL_DEBUG, "%s [%u]: Button %u released %u time(s). (%u, %u)\n",
            __FUNCTION__, __LINE__, event.data.mouse.button, event.data.mouse.clicks,
            event.data.mouse.x, event.data.mouse.y);

    // Fire mouse released event.
    dispatch_event(&event);

    // If the pressed event was not consumed...
    if (event.reserved ^ 0x01 && mouse_dragged != true) {
        // Populate mouse clicked event.
        event.time = timestamp;
        event.reserved = 0x00;

        event.type = EVENT_MOUSE_CLICKED;
        event.mask = get_modifiers();

        event.data.mouse.button = button;
        event.data.mouse.clicks = click_count;
        event.data.mouse.x = event_point.x;
        event.data.mouse.y = event_point.y;

        logger(LOG_LEVEL_DEBUG, "%s [%u]: Button %u clicked %u time(s). (%u, %u)\n",
                __FUNCTION__, __LINE__, event.data.mouse.button, event.data.mouse.clicks,
                event.data.mouse.x, event.data.mouse.y);

        // Fire mouse clicked event.
        dispatch_event(&event);
    }

    // Reset the number of clicks.
    if ((long int) (timestamp - click_time) / 1000000 > hook_get_multi_click_time()) {
        // Reset the click count.
        click_count = 0;
    }
}

static inline void process_mouse_moved(uint64_t timestamp, CGEventRef event_ref) {
    // Reset the click count.
    if (click_count != 0 && (long int) (timestamp - click_time) / 1000000 > hook_get_multi_click_time()) {
        click_count = 0;
    }

    CGPoint event_point = CGEventGetLocation(event_ref);

    // Populate mouse motion event.
    event.time = timestamp;
    event.reserved = 0x00;

    if (mouse_dragged) {
        event.type = EVENT_MOUSE_DRAGGED;
    }
    else {
        event.type = EVENT_MOUSE_MOVED;
    }
    event.mask = get_modifiers();

    event.data.mouse.button = MOUSE_NOBUTTON;
    event.data.mouse.clicks = click_count;
    event.data.mouse.x = event_point.x;
    event.data.mouse.y = event_point.y;

    logger(LOG_LEVEL_DEBUG, "%s [%u]: Mouse %s to %u, %u.\n",
            __FUNCTION__, __LINE__, mouse_dragged ? "dragged" : "moved",
            event.data.mouse.x, event.data.mouse.y);

    // Fire mouse motion event.
    dispatch_event(&event);
}

static inline void process_mouse_wheel(uint64_t timestamp, CGEventRef event_ref) {
    // Reset the click count and previous button.
    click_count = 1;
    click_button = MOUSE_NOBUTTON;

    // Check to see what axis was rotated, we only care about axis 1 for vertical rotation.
    // TODO Implement horizontal scrolling by examining axis 2.
    // NOTE kCGScrollWheelEventDeltaAxis3 is currently unused.
    if (CGEventGetIntegerValueField(event_ref, kCGScrollWheelEventDeltaAxis1) != 0
            || CGEventGetIntegerValueField(event_ref, kCGScrollWheelEventDeltaAxis2) != 0) {
        CGPoint event_point = CGEventGetLocation(event_ref);

        // Populate mouse wheel event.
        event.time = timestamp;
        event.reserved = 0x00;

        event.type = EVENT_MOUSE_WHEEL;
        event.mask = get_modifiers();

        event.data.wheel.clicks = click_count;
        event.data.wheel.x = event_point.x;
        event.data.wheel.y = event_point.y;

        // TODO Figure out if kCGScrollWheelEventDeltaAxis2 causes mouse events with zero rotation.
        if (CGEventGetIntegerValueField(event_ref, kCGScrollWheelEventIsContinuous) == 0) {
            // Scrolling data is line-based.
            event.data.wheel.type = WHEEL_BLOCK_SCROLL;
        } else {
            // Scrolling data is pixel-based.
            event.data.wheel.type = WHEEL_UNIT_SCROLL;
        }

        // TODO The result of kCGScrollWheelEventIsContinuous may effect this value.
        // Calculate the amount based on the Point Delta / Event Delta.  Integer sign should always be homogeneous resulting in a positive result.
        // NOTE kCGScrollWheelEventFixedPtDeltaAxis1 a floating point value (+0.1/-0.1) that takes acceleration into account.
        // NOTE kCGScrollWheelEventPointDeltaAxis1 will not build on OS X < 10.5

        if (CGEventGetIntegerValueField(event_ref, kCGScrollWheelEventDeltaAxis1) != 0) {
            event.data.wheel.amount = CGEventGetIntegerValueField(event_ref, kCGScrollWheelEventPointDeltaAxis1) / CGEventGetIntegerValueField(event_ref, kCGScrollWheelEventDeltaAxis1);

            // Scrolling data uses a fixed-point 16.16 signed integer format (Ex: 1.0 = 0x00010000).
            event.data.wheel.rotation = CGEventGetIntegerValueField(event_ref, kCGScrollWheelEventDeltaAxis1) * -1;

        } else if (CGEventGetIntegerValueField(event_ref, kCGScrollWheelEventDeltaAxis2) != 0) {
            event.data.wheel.amount = CGEventGetIntegerValueField(event_ref, kCGScrollWheelEventPointDeltaAxis2) / CGEventGetIntegerValueField(event_ref, kCGScrollWheelEventDeltaAxis2);

            // Scrolling data uses a fixed-point 16.16 signed integer format (Ex: 1.0 = 0x00010000).
            event.data.wheel.rotation = CGEventGetIntegerValueField(event_ref, kCGScrollWheelEventDeltaAxis2) * -1;
        } else {
            //Fail Silently if a 3rd axis gets added without changing this section of code.
            event.data.wheel.amount = 0;
            event.data.wheel.rotation = 0;
        }


        if (CGEventGetIntegerValueField(event_ref, kCGScrollWheelEventDeltaAxis1) != 0) {
            // Wheel Rotated Up or Down.
            event.data.wheel.direction = WHEEL_VERTICAL_DIRECTION;
        } else { // data->event.u.u.detail == WheelLeft || data->event.u.u.detail == WheelRight
            // Wheel Rotated Left or Right.
            event.data.wheel.direction = WHEEL_HORIZONTAL_DIRECTION;
        }

        logger(LOG_LEVEL_DEBUG, "%s [%u]: Mouse wheel type %u, rotated %i units in the %u direction at %u, %u.\n",
                __FUNCTION__, __LINE__, event.data.wheel.type,
                event.data.wheel.amount * event.data.wheel.rotation,
                event.data.wheel.direction,
                event.data.wheel.x, event.data.wheel.y);

        // Fire mouse wheel event.
        dispatch_event(&event);
    }
}

CGEventRef hook_event_proc(CGEventTapProxy tap_proxy, CGEventType type, CGEventRef event_ref, void *refcon) {
    // Get the local system time in UTC.
    gettimeofday(&system_time, NULL);

    // Grab the native event timestamp for use later..
    uint64_t timestamp = (uint64_t) CGEventGetTimestamp(event_ref);

    // Get the event class.
    switch (type) {
        case kCGEventKeyDown:
            process_key_pressed(timestamp, event_ref);
            break;

        case kCGEventKeyUp:
            process_key_released(timestamp, event_ref);
            break;

        case kCGEventFlagsChanged:
            process_modifier_changed(timestamp, event_ref);
            break;

        case NX_SYSDEFINED:
            process_system_key(timestamp, event_ref);
            break;

        case kCGEventLeftMouseDown:
            set_modifier_mask(MASK_BUTTON1);
            process_button_pressed(timestamp, event_ref, MOUSE_BUTTON1);
            break;

        case kCGEventRightMouseDown:
            set_modifier_mask(MASK_BUTTON2);
            process_button_pressed(timestamp, event_ref, MOUSE_BUTTON2);
            break;

        case kCGEventOtherMouseDown:
            // Extra mouse buttons.
            if (CGEventGetIntegerValueField(event_ref, kCGMouseEventButtonNumber) < UINT16_MAX) {
                uint16_t button = (uint16_t) CGEventGetIntegerValueField(event_ref, kCGMouseEventButtonNumber) + 1;

                // Add support for mouse 4 & 5.
                if (button == 4) {
                    set_modifier_mask(MOUSE_BUTTON4);
                } else if (button == 5) {
                    set_modifier_mask(MOUSE_BUTTON5);
                }

                process_button_pressed(timestamp, event_ref, button);
            }
            break;

        case kCGEventLeftMouseUp:
            unset_modifier_mask(MASK_BUTTON1);
            process_button_released(timestamp, event_ref, MOUSE_BUTTON1);
            break;

        case kCGEventRightMouseUp:
            unset_modifier_mask(MASK_BUTTON2);
            process_button_released(timestamp, event_ref, MOUSE_BUTTON2);
            break;

        case kCGEventOtherMouseUp:
            // Extra mouse buttons.
            if (CGEventGetIntegerValueField(event_ref, kCGMouseEventButtonNumber) < UINT16_MAX) {
                uint16_t button = (uint16_t) CGEventGetIntegerValueField(event_ref, kCGMouseEventButtonNumber) + 1;

                // Add support for mouse 4 & 5.
                if (button == 4) {
                    unset_modifier_mask(MOUSE_BUTTON4);
                } else if (button == 5) {
                    unset_modifier_mask(MOUSE_BUTTON5);
                }

                process_button_pressed(timestamp, event_ref, button);
            }
            break;


        case kCGEventLeftMouseDragged:
        case kCGEventRightMouseDragged:
        case kCGEventOtherMouseDragged:
            // FIXME The drag flag is confusing.  Use prev x,y to determine click.
            // Set the mouse dragged flag.
            mouse_dragged = true;
            process_mouse_moved(timestamp, event_ref);
            break;

        case kCGEventMouseMoved:
            // Set the mouse dragged flag.
            mouse_dragged = false;
            process_mouse_moved(timestamp, event_ref);
            break;


        case kCGEventScrollWheel:
            process_mouse_wheel(timestamp, event_ref);
            break;

        default:
            // Check for an old OS X bug where the tap seems to timeout for no reason.
            // See: http://stackoverflow.com/questions/2969110/cgeventtapcreate-breaks-down-mysteriously-with-key-down-events#2971217
            if (type == (CGEventType) kCGEventTapDisabledByTimeout) {
                logger(LOG_LEVEL_WARN, "%s [%u]: CGEventTap timeout!\n",
                        __FUNCTION__, __LINE__);

                // We need to restart the tap!
                restart_tap = true;
                CFRunLoopStop(CFRunLoopGetCurrent());
            } else {
                // In theory this *should* never execute.
                logger(LOG_LEVEL_DEBUG, "%s [%u]: Unhandled Darwin event: %#X.\n",
                        __FUNCTION__, __LINE__, (unsigned int) type);
            }
            break;
    }

    CGEventRef result_ref = NULL;
    if (event.reserved ^ 0x01) {
        result_ref = event_ref;
    } else {
        logger(LOG_LEVEL_DEBUG, "%s [%u]: Consuming the current event. (%#X) (%#p)\n",
                __FUNCTION__, __LINE__, type, event_ref);
    }

    return result_ref;
}


#if !defined(USE_CARBON_LEGACY) && defined(USE_APPLICATION_SERVICES)
void main_runloop_status_proc(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *info) {
    switch (activity) {
        case kCFRunLoopExit:
            // Acquire a lock on the msg_port and signal that anyone waiting should continue.
            pthread_mutex_lock(&main_runloop_mutex);
            pthread_cond_broadcast(&main_runloop_cond);
            pthread_mutex_unlock(&main_runloop_mutex);
            break;
    }
}

// Runloop to execute KeyCodeToString on the "Main" runloop due to an undocumented thread safety requirement.
static void main_runloop_keycode_proc(void *info) {
    // Lock the msg_port mutex as we enter the main runloop.
    pthread_mutex_lock(&main_runloop_mutex);

    keycode_to_lookup(info);

    // Unlock the msg_port mutex to signal to the hook_thread that we have
    // finished on the main runloop.
    pthread_cond_broadcast(&main_runloop_cond);
    pthread_mutex_unlock(&main_runloop_mutex);
}

static int create_main_runloop_info(main_runloop_info **main, CFRunLoopSourceContext *context) {
    if (*main != NULL) {
        logger(LOG_LEVEL_ERROR, "%s [%u]: Expected unallocated main_runloop_info pointer!\n",
                __FUNCTION__, __LINE__);

        return UIOHOOK_FAILURE;
    }

    // Try and allocate memory for event_runloop_info.
    *main = malloc(sizeof(main_runloop_info));
    if (*main == NULL) {
        logger(LOG_LEVEL_ERROR, "%s [%u]: Failed to allocate memory for main_runloop_info structure!\n",
                __FUNCTION__, __LINE__);

        return UIOHOOK_ERROR_OUT_OF_MEMORY;
    }

    // Create a runloop observer for the main runloop.
    (*main)->observer = CFRunLoopObserverCreate(
            kCFAllocatorDefault,
            kCFRunLoopExit, //kCFRunLoopEntry | kCFRunLoopExit, //kCFRunLoopAllActivities,
            true,
            0,
            main_runloop_status_proc,
            NULL
        );
    if ((*main)->observer == NULL) {
        logger(LOG_LEVEL_ERROR, "%s [%u]: CFRunLoopObserverCreate failure!\n",
                __FUNCTION__, __LINE__);

        return UIOHOOK_ERROR_CREATE_OBSERVER;
    } else {
        logger(LOG_LEVEL_DEBUG, "%s [%u]: CFRunLoopObserverCreate success!\n",
                __FUNCTION__, __LINE__);
    }

    (*main)->source = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, context);

    if ((*main)->source == NULL) {
        logger(LOG_LEVEL_ERROR, "%s [%u]: CFRunLoopSourceCreate failure!\n",
                __FUNCTION__, __LINE__);

        return UIOHOOK_ERROR_CREATE_RUN_LOOP_SOURCE;
    } else {
        logger(LOG_LEVEL_DEBUG, "%s [%u]: CFRunLoopSourceCreate success!\n",
                __FUNCTION__, __LINE__);
    }

    // FIXME Check for null ?
    CFRunLoopRef main_loop = CFRunLoopGetMain();

    pthread_mutex_lock(&main_runloop_mutex);

    CFRunLoopAddSource(main_loop, (*main)->source, kCFRunLoopDefaultMode);
    CFRunLoopAddObserver(main_loop, (*main)->observer, kCFRunLoopDefaultMode);

    pthread_mutex_unlock(&main_runloop_mutex);

    return UIOHOOK_SUCCESS;
}

static void destroy_main_runloop_info(main_runloop_info **main) {
    if (*main != NULL) {
         CFRunLoopRef main_loop = CFRunLoopGetMain();

         if ((*main)->observer != NULL) {
             if (CFRunLoopContainsObserver(main_loop, (*main)->observer, kCFRunLoopDefaultMode)) {
                 CFRunLoopRemoveObserver(main_loop, (*main)->observer, kCFRunLoopDefaultMode);
             }

             CFRunLoopObserverInvalidate((*main)->observer);
             CFRelease((*main)->observer);
             (*main)->observer = NULL;
         }

         if ((*main)->source != NULL) {
             if (CFRunLoopContainsSource(main_loop, (*main)->source, kCFRunLoopDefaultMode)) {
                 CFRunLoopRemoveSource(main_loop, (*main)->source, kCFRunLoopDefaultMode);
             }

             CFRelease((*main)->source);
             (*main)->source = NULL;
         }

        // Free the main structure.
        free(*main);
        *main = NULL;
    }
}
#endif


static int create_event_runloop_info(event_runloop_info **hook) {
    if (*hook != NULL) {
        logger(LOG_LEVEL_ERROR, "%s [%u]: Expected unallocated event_runloop_info pointer!\n",
                __FUNCTION__, __LINE__);

        return UIOHOOK_FAILURE;
    }

    // Try and allocate memory for event_runloop_info.
    *hook = malloc(sizeof(event_runloop_info));
    if (*hook == NULL) {
        logger(LOG_LEVEL_ERROR, "%s [%u]: Failed to allocate memory for event_runloop_info structure!\n",
                __FUNCTION__, __LINE__);

        return UIOHOOK_ERROR_OUT_OF_MEMORY;
    }

    // Setup the event mask to listen for.
    CGEventMask event_mask = CGEventMaskBit(kCGEventKeyDown) |
            CGEventMaskBit(kCGEventKeyUp) |
            CGEventMaskBit(kCGEventFlagsChanged) |

            CGEventMaskBit(kCGEventLeftMouseDown) |
            CGEventMaskBit(kCGEventLeftMouseUp) |
            CGEventMaskBit(kCGEventLeftMouseDragged) |

            CGEventMaskBit(kCGEventRightMouseDown) |
            CGEventMaskBit(kCGEventRightMouseUp) |
            CGEventMaskBit(kCGEventRightMouseDragged) |

            CGEventMaskBit(kCGEventOtherMouseDown) |
            CGEventMaskBit(kCGEventOtherMouseUp) |
            CGEventMaskBit(kCGEventOtherMouseDragged) |

            CGEventMaskBit(kCGEventMouseMoved) |
            CGEventMaskBit(kCGEventScrollWheel) |

            // NOTE This event is undocumented and used
            // for caps-lock release and multi-media keys.
            CGEventMaskBit(NX_SYSDEFINED);

    // Create the event tap.
    (*hook)->port = CGEventTapCreate(
            kCGSessionEventTap,       // kCGHIDEventTap
            kCGHeadInsertEventTap,    // kCGTailAppendEventTap
            kCGEventTapOptionDefault, // kCGEventTapOptionListenOnly See https://github.com/kwhat/jnativehook/issues/22
            event_mask,
            hook_event_proc,
            NULL);
    if ((*hook)->port == NULL) {
        logger(LOG_LEVEL_ERROR, "%s [%u]: Failed to create event port!\n",
                __FUNCTION__, __LINE__);

        return UIOHOOK_ERROR_CREATE_EVENT_PORT;
    } else {
        logger(LOG_LEVEL_DEBUG, "%s [%u]: CGEventTapCreate Successful.\n",
                __FUNCTION__, __LINE__);
    }

    // Create the runloop event source from the event tap.
    (*hook)->source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, (*hook)->port, 0);
    if ((*hook)->source == NULL) {
        logger(LOG_LEVEL_ERROR, "%s [%u]: CFMachPortCreateRunLoopSource failure!\n",
                __FUNCTION__, __LINE__);

        return UIOHOOK_ERROR_CREATE_RUN_LOOP_SOURCE;
    } else {
        logger(LOG_LEVEL_DEBUG, "%s [%u]: CFMachPortCreateRunLoopSource successful.\n",
                __FUNCTION__, __LINE__);
    }

    // Create run loop observers.
    (*hook)->observer = CFRunLoopObserverCreate(
            kCFAllocatorDefault,
            kCFRunLoopEntry | kCFRunLoopExit, //kCFRunLoopAllActivities,
            true,
            0,
            hook_status_proc,
            NULL);
    if ((*hook)->observer == NULL) {
        logger(LOG_LEVEL_ERROR, "%s [%u]: CFRunLoopObserverCreate failure!\n",
                __FUNCTION__, __LINE__);

        return UIOHOOK_ERROR_CREATE_OBSERVER;
    } else {
        logger(LOG_LEVEL_DEBUG, "%s [%u]: CFRunLoopObserverCreate successful.\n",
                __FUNCTION__, __LINE__);
    }

    // Add the event source and observer to the runloop mode.
    CFRunLoopAddSource(event_loop, (*hook)->source, kCFRunLoopDefaultMode);
    CFRunLoopAddObserver(event_loop, (*hook)->observer, kCFRunLoopDefaultMode);

    return UIOHOOK_SUCCESS;
}

static void destroy_event_runloop_info(event_runloop_info **hook) {
    // FIXME check event_loop for null ?

    if (*hook != NULL) {
        if ((*hook)->observer != NULL) {
            if (CFRunLoopContainsObserver(event_loop, (*hook)->observer, kCFRunLoopDefaultMode)) {
                CFRunLoopRemoveObserver(event_loop, (*hook)->observer, kCFRunLoopDefaultMode);
            }

            // Invalidate and free hook observer.
            CFRunLoopObserverInvalidate((*hook)->observer);
            CFRelease((*hook)->observer);
            (*hook)->observer = NULL;
        }

        if ((*hook)->source != NULL) {
            if (CFRunLoopContainsSource(event_loop, (*hook)->source, kCFRunLoopDefaultMode)) {
                CFRunLoopRemoveSource(event_loop, (*hook)->source, kCFRunLoopDefaultMode);
            }

            // Clean up the event source.
            CFRelease((*hook)->source);
            (*hook)->source = NULL;
        }

        // Stop the CFMachPort from receiving any more messages.
        CFMachPortInvalidate((*hook)->port);
        CFRelease((*hook)->port);
        (*hook)->port = NULL;

        // Free the hook structure.
        free(*hook);
        *hook = NULL;
    }
}

UIOHOOK_API int hook_run() {
    int status = UIOHOOK_SUCCESS;

    // Check for accessibility before we start the loop.
    if (is_accessibility_enabled()) {
        logger(LOG_LEVEL_DEBUG, "%s [%u]: Accessibility API is enabled.\n",
                __FUNCTION__, __LINE__);

        event_loop = CFRunLoopGetCurrent();
        if (event_loop != NULL) {
            logger(LOG_LEVEL_DEBUG, "%s [%u]: CFRunLoopGetCurrent successful.\n",
                    __FUNCTION__, __LINE__);

            do {
                // Reset the restart flag...
                restart_tap = false;

                // Initialize starting modifiers.
                initialize_modifiers();

                // Try and allocate memory for event_runloop_info.
                event_runloop_info *hook = NULL;
                int event_runloop_status = create_event_runloop_info(&hook);
                if (event_runloop_status != UIOHOOK_SUCCESS) {
                    destroy_event_runloop_info(&hook);
                    return event_runloop_status;
                }


                tis_keycode_message = (TISKeycodeMessage *) calloc(1, sizeof(TISKeycodeMessage));
                if (tis_keycode_message == NULL) {
                    logger(LOG_LEVEL_ERROR, "%s [%u]: Failed to allocate memory for TIS message structure!\n",
                                __FUNCTION__, __LINE__);

                    return UIOHOOK_ERROR_OUT_OF_MEMORY;
                }

                #ifdef USE_OBJC
                tis_event_message = (TISEventMessage *) calloc(1, sizeof(TISEventMessage));
                if (tis_event_message == NULL) {
                    logger(LOG_LEVEL_ERROR, "%s [%u]: Failed to allocate memory for TIS event structure!\n",
                                __FUNCTION__, __LINE__);

                    return UIOHOOK_ERROR_OUT_OF_MEMORY;
                }
                #endif

                // If we are not running on the main runloop, we need to setup a runloop dispatcher.
                if (!CFEqual(event_loop, CFRunLoopGetMain())) {
                    // Dynamically load dispatch_sync_f to maintain 10.5 compatibility.
                    *(void **) (&dispatch_sync_f_f) = dlsym(RTLD_DEFAULT, "dispatch_sync_f");
                    const char *dlError = dlerror();
                    if (dlError != NULL) {
                        logger(LOG_LEVEL_DEBUG, "%s [%u]: %s.\n",
                                __FUNCTION__, __LINE__, dlError);
                    }

                    // This load is equivalent to calling dispatch_get_main_queue().  We use
                    // _dispatch_main_q because dispatch_get_main_queue is not exported from
                    // libdispatch.dylib and the upstream function only dereferences the pointer.
                    dispatch_main_queue_s = (struct dispatch_queue_s *) dlsym(RTLD_DEFAULT, "_dispatch_main_q");
                    dlError = dlerror();
                    if (dlError != NULL) {
                        logger(LOG_LEVEL_DEBUG, "%s [%u]: %s.\n",
                                __FUNCTION__, __LINE__, dlError);
                    }

                    if (dispatch_sync_f_f == NULL || dispatch_main_queue_s == NULL) {
                        logger(LOG_LEVEL_DEBUG, "%s [%u]: Failed to locate dispatch_sync_f() or dispatch_get_main_queue()!\n",
                                __FUNCTION__, __LINE__);

                        #if !defined(USE_CARBON_LEGACY) && defined(USE_APPLICATION_SERVICES)
                        logger(LOG_LEVEL_DEBUG, "%s [%u]: Falling back to runloop signaling.\n",
                                __FUNCTION__, __LINE__);

                        // TODO The only thing that maybe needed in this struct is the .perform
                        CFRunLoopSourceContext main_runloop_keycode_context = {
                            .version         = 0,
                            .info            = tis_keycode_message,
                            .retain          = NULL,
                            .release         = NULL,
                            .copyDescription = NULL,
                            .equal           = NULL,
                            .hash            = NULL,
                            .schedule        = NULL,
                            .cancel          = NULL,
                            .perform         = main_runloop_keycode_proc
                        };

                        int keycode_runloop_status = create_main_runloop_info(&main_runloop_keycode, &main_runloop_keycode_context);
                        if (keycode_runloop_status != UIOHOOK_SUCCESS) {
                            destroy_main_runloop_info(&main_runloop_keycode);
                            return keycode_runloop_status;
                        }
                        #endif
                    }
                }

                #ifdef USE_OBJC
                // Contributed by Alex <universailp@web.de>
                // Create a garbage collector to handle Cocoa events correctly.
                Class NSAutoreleasePool_class = (Class) objc_getClass("NSAutoreleasePool");
                id pool = class_createInstance(NSAutoreleasePool_class, 0);

                id (*eventWithoutCGEvent)(id, SEL) = (id (*)(id, SEL)) objc_msgSend;
                auto_release_pool = eventWithoutCGEvent(pool, sel_registerName("init"));
                #endif


                // Start the hook thread runloop.
                CFRunLoopRun();


                #ifdef USE_OBJC
                // Contributed by Alex <universailp@web.de>
                eventWithoutCGEvent(auto_release_pool, sel_registerName("release"));
                #endif

                #if !defined(USE_CARBON_LEGACY) && defined(USE_APPLICATION_SERVICES)
                pthread_mutex_lock(&main_runloop_mutex);
                if (!CFEqual(event_loop, CFRunLoopGetMain())) {
                    if (dispatch_sync_f_f == NULL || dispatch_main_queue_s == NULL) {
                        destroy_main_runloop_info(&main_runloop_keycode);
                    }
                }
                pthread_mutex_unlock(&main_runloop_mutex);
                #endif

                #ifdef USE_OBJC
                free(tis_event_message);
                #endif
                free(tis_keycode_message);

                destroy_event_runloop_info(&hook);
            } while (restart_tap);
        } else {
            logger(LOG_LEVEL_ERROR, "%s [%u]: CFRunLoopGetCurrent failure!\n",
                    __FUNCTION__, __LINE__);

            status = UIOHOOK_ERROR_GET_RUNLOOP;
        }
    } else {
        logger(LOG_LEVEL_ERROR, "%s [%u]: Accessibility API is disabled!\n",
                __FUNCTION__, __LINE__);

        status = UIOHOOK_ERROR_AXAPI_DISABLED;
    }

    logger(LOG_LEVEL_DEBUG, "%s [%u]: Something, something, something, complete.\n",
            __FUNCTION__, __LINE__);

    return status;
}

UIOHOOK_API int hook_stop() {
    int status = UIOHOOK_FAILURE;

    CFStringRef mode = CFRunLoopCopyCurrentMode(event_loop);
    if (mode != NULL) {
        CFRelease(mode);

        // Make sure the tap doesn't restart.
        restart_tap = false;

        // Stop the run loop.
        CFRunLoopStop(event_loop);

        // Cleanup native input functions.
        unload_input_helper();

        status = UIOHOOK_SUCCESS;
    }

    logger(LOG_LEVEL_DEBUG, "%s [%u]: Status: %#X.\n",
            __FUNCTION__, __LINE__, status);

    return status;
}
