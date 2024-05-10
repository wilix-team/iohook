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

#ifdef USE_CARBON_LEGACY
#include <Carbon/Carbon.h>
#endif

#if defined(USE_APPLICATION_SERVICES) || defined(USE_IOKIT)
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef USE_IOKIT
#include <IOKit/hidsystem/event_status_driver.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#endif

#include <stdbool.h>
#include <uiohook.h>

#include "logger.h"
#include "input_helper.h"

#ifdef USE_IOKIT
static io_connect_t connection;
#endif

#define MOUSE_ACCELERATION_MULTIPLIER 65536

/* The following function was contributed by Anthony Liguori Jan 18 2015.
 * https://github.com/kwhat/libuiohook/pull/18
 */
UIOHOOK_API screen_data* hook_create_screen_info(unsigned char *count) {
    CGError status = kCGErrorFailure;
    screen_data* screens = NULL;

    // Initialize count to zero.
    *count = 0;

    // Allocate memory to hold each display id.  We will just allocate our MAX
    // because its only about 1K of memory.
    // TODO This can probably be realistically cut to something like 16 or 32....
    // If you have more than 32 monitors, send me a picture and make a donation ;)
    CGDirectDisplayID *display_ids = malloc(sizeof(CGDirectDisplayID) * UCHAR_MAX);
    if (display_ids != NULL) {
        // NOTE Pass UCHAR_MAX to make sure uint32_t doesn't overflow uint8_t.
        // TOOD Test/Check whether CGGetOnlineDisplayList is more suitable...
        status = CGGetActiveDisplayList(UCHAR_MAX, display_ids, (uint32_t *) count);

        // If there is no error and at least one monitor.
        if (status == kCGErrorSuccess && *count > 0) {
            logger(LOG_LEVEL_DEBUG, "%s [%u]: CGGetActiveDisplayList: %li.\n",
                    __FUNCTION__, __LINE__, *count);

            // Allocate memory for the number of screens found.
            screens = malloc(sizeof(screen_data) * (*count));
            if (screens != NULL) {
                for (uint8_t i = 0; i < *count; i++) {
                    //size_t width = CGDisplayPixelsWide(display_ids[i]);
                    //size_t height = CGDisplayPixelsHigh(display_ids[i]);
                    CGRect boundsDisp = CGDisplayBounds(display_ids[i]);
                    if (boundsDisp.size.width > 0 && boundsDisp.size.height > 0) {
                        screens[i] = (screen_data) {
                            .number = i + 1,
                            //TODO: make sure we follow the same convention for the origin
                            //in all other platform implementations (upper-left)
                            //TODO: document the approach with examples in order to show different
                            //cases -> different resolutions (secondary monitors origin might be
                            //negative)
                            .x = boundsDisp.origin.x,
                            .y = boundsDisp.origin.y,
                            .width = boundsDisp.size.width,
                            .height = boundsDisp.size.height
                        };
                }
                }
            }
        } else {
            logger(LOG_LEVEL_WARN, "%s [%u]: multiple_get_screen_info failed: %ld. Fallback.\n",
                    __FUNCTION__, __LINE__, status);

            size_t width = CGDisplayPixelsWide(CGMainDisplayID());
            size_t height = CGDisplayPixelsHigh(CGMainDisplayID());

            if (width > 0 && height > 0) {
                screens = malloc(sizeof(screen_data));

                if (screens != NULL) {
                    *count = 1;
                    screens[0] = (screen_data) {
                        .number = 1,
                        .x = 0,
                        .y = 0,
                        .width = width,
                        .height = height
                    };
                }
            }
        }

        // Free the id's after we are done.
        free(display_ids);
    }

    return screens;
}

/*
 * Apple's documentation is not very good.  I was finally able to find this
 * information after many hours of googling.  Value is the slider value in the
 * system preferences. That value * 15 is the rate in MS.  66 / the value is the
 * chars per second rate.
 *
 * Value    MS      Char/Sec
 *
 * 1        15      66        * Out of standard range *
 * 2        30      33
 * 6        90      11
 * 12       180     5.5
 * 30       450     2.2
 * 60       900     1.1
 * 90       1350    0.73
 * 120      1800    0.55
 *
 * V = MS / 15
 * V = 66 / CharSec
 *
 * MS = V * 15
 * MS = (66 / CharSec) * 15
 *
 * CharSec = 66 / V
 * CharSec = 66 / (MS / 15)
 */
UIOHOOK_API long int hook_get_auto_repeat_rate() {
    #if defined(USE_APPLICATION_SERVICES) || defined(USE_IOKIT) || defined(USE_CARBON_LEGACY)
    bool successful = false;
    SInt64 rate;
    #endif

    long int value = -1;

    #ifdef USE_APPLICATION_SERVICES
    if (!successful) {
        CFTypeRef pref_val = CFPreferencesCopyValue(CFSTR("KeyRepeat"), kCFPreferencesAnyApplication, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
        if (pref_val != NULL) {
            if (CFGetTypeID(pref_val) == CFNumberGetTypeID() && CFNumberGetValue((CFNumberRef) pref_val, kCFNumberSInt64Type, &rate)) {
                // This is the slider value, we must multiply by 15 to convert to milliseconds.
                value = (long) rate * 15;
                successful = true;

                logger(LOG_LEVEL_DEBUG, "%s [%u]: CFPreferencesCopyValue KeyRepeat: %li.\n",
                        __FUNCTION__, __LINE__, rate);
            }

            CFRelease(pref_val);
        }
    }
    #endif

    #ifdef USE_IOKIT
    if (!successful) {
        CFTypeRef cf_type = NULL;
        kern_return_t kern_return = IOHIDCopyCFTypeParameter(connection, CFSTR(kIOHIDKeyRepeatKey), &cf_type);
        if (kern_return == kIOReturnSuccess) {
            if (cf_type != NULL) {
                if (CFGetTypeID(cf_type) == CFNumberGetTypeID()) {
                    if (CFNumberGetValue((CFNumberRef) cf_type, kCFNumberSInt64Type, &rate)) {
                        /* This is in some undefined unit of time that if we happen
                         * to multiply by 900 gives us the time in milliseconds. We
                         * add 0.5 to the result so that when we cast to long we
                         * actually get a rounded result.  Saves the math.h depend.
                         *
                         * 900 *    33,333,333 / 1000.0 / 1000.0 / 1000.0 + 0.5 == 30    * Fast *
                         * 900 *   100,000,000 / 1000.0 / 1000.0 / 1000.0 + 0.5 == 90
                         * 900 *   200,000,000 / 1000.0 / 1000.0 / 1000.0 + 0.5 == 180
                         * 900 *   500,000,000 / 1000.0 / 1000.0 / 1000.0 + 0.5 == 450
                         * 900 * 1,000,000,000 / 1000.0 / 1000.0 / 1000.0 + 0.5 == 900
                         * 900 * 1,500,000,000 / 1000.0 / 1000.0 / 1000.0 + 0.5 == 1350
                         * 900 * 2,000,000,000 / 1000.0 / 1000.0 / 1000.0 + 0.5 == 1800  * Slow *
                         */
                        value = (long) (900 * rate / 1000.0 / 1000.0 / 1000.0 + 0.5);
                        successful = true;

                        logger(LOG_LEVEL_DEBUG, "%s [%u]: IORegistryEntryCreateCFProperty kIOHIDKeyRepeatKey: %li.\n",
                                __FUNCTION__, __LINE__, value);
                    }
                }
                
                CFRelease(cf_type);
            }
        }
    }
    #endif

    #ifdef USE_CARBON_LEGACY
    if (!successful) {
        // Apple documentation states that value is in 'ticks'. I am not sure
        // what that means, but it looks a lot like the arbitrary slider value.
        rate = LMGetKeyRepThresh();
        if (rate > -1) {
            /* This is the slider value, we must multiply by 15 to convert to
             * milliseconds.
             */
            value = (long) rate * 15;
            successful = true;

            logger(LOG_LEVEL_DEBUG, "%s [%u]: LMGetKeyRepThresh: %li.\n",
                    __FUNCTION__, __LINE__, value);
        }
    }
    #endif

    return value;
}

UIOHOOK_API long int hook_get_auto_repeat_delay() {
    #if defined(USE_APPLICATION_SERVICES) || defined(USE_IOKIT) || defined(USE_CARBON_LEGACY)
    bool successful = false;
    SInt64 delay;
    #endif

    long int value = -1;

    #ifdef USE_APPLICATION_SERVICES
    if (!successful) {
        CFTypeRef pref_val = CFPreferencesCopyValue(CFSTR("InitialKeyRepeat"), kCFPreferencesAnyApplication, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
        if (pref_val != NULL) {
            if (CFGetTypeID(pref_val) == CFNumberGetTypeID() && CFNumberGetValue((CFNumberRef) pref_val, kCFNumberSInt64Type, &delay)) {
                // This is the slider value, we must multiply by 15 to convert to milliseconds.
                value = (long) delay * 15;
                successful = true;

                logger(LOG_LEVEL_DEBUG, "%s [%u]: CFPreferencesCopyValue InitialKeyRepeat: %li.\n",
                        __FUNCTION__, __LINE__, value);
            }

            CFRelease(pref_val);
        }
    }
    #endif

    #ifdef USE_IOKIT
    if (!successful) {
        CFTypeRef cf_type = NULL;
        kern_return_t kern_return = IOHIDCopyCFTypeParameter(connection, CFSTR(kIOHIDInitialKeyRepeatKey), &cf_type);
        if (kern_return == kIOReturnSuccess) {
            if (cf_type != NULL) {
                if (CFGetTypeID(cf_type) == CFNumberGetTypeID()) {
                    if (CFNumberGetValue((CFNumberRef) cf_type, kCFNumberSInt64Type, &delay)) {
                        /* This is in some undefined unit of time that if we happen
                         * to multiply by 900 gives us the time in milliseconds. We
                         * add 0.5 to the result so that when we cast to long we
                         * actually get a rounded result.  Saves the math.h depend.
                         *
                         * 900 *   250,000,000 / 1000.0 / 1000.0 / 1000.0 + 0.5 == 225   * Fast *
                         * 900 *   416,666,666 / 1000.0 / 1000.0 / 1000.0 + 0.5 == 375
                         * 900 *   583,333,333 / 1000.0 / 1000.0 / 1000.0 + 0.5 == 525
                         * 900 * 1,133,333,333 / 1000.0 / 1000.0 / 1000.0 + 0.5 == 1020
                         * 900 * 1,566,666,666 / 1000.0 / 1000.0 / 1000.0 + 0.5 == 1410
                         * 900 * 2,000,000,000 / 1000.0 / 1000.0 / 1000.0 + 0.5 == 1800  * Slow *
                         */
                        value = (long) (900 * delay / 1000.0 / 1000.0 / 1000.0 + 0.5);
                        successful = true;

                        logger(LOG_LEVEL_DEBUG, "%s [%u]: IORegistryEntryCreateCFProperty kIOHIDInitialKeyRepeatKey: %li.\n",
                                __FUNCTION__, __LINE__, delay);
                    }
                }

                CFRelease(cf_type);
            }
        }
    }
    #endif

    #ifdef USE_CARBON_LEGACY
    if (!successful) {
        // Apple documentation states that value is in 'ticks'. I am not sure
        // what that means, but it looks a lot like the arbitrary slider value.
        delay = LMGetKeyThresh();
        if (delay > -1) {
            // This is the slider value, we must multiply by 15 to convert to
            // milliseconds.
            value = (long) delay * 15;
            successful = true;

            logger(LOG_LEVEL_DEBUG, "%s [%u]: LMGetKeyThresh: %li.\n",
                    __FUNCTION__, __LINE__, value);
        }
    }
    #endif

    return value;
}

UIOHOOK_API long int hook_get_pointer_acceleration_multiplier() {
    // OS X doesn't currently have an acceleration multiplier so we are using the constant from IOHIDGetMouseAcceleration.
    long int value = MOUSE_ACCELERATION_MULTIPLIER;
    if (hook_get_pointer_sensitivity() < 0) {
        value = 0;
    }

    return value;
}

UIOHOOK_API long int hook_get_pointer_acceleration_threshold() {
    // OS X doesn't currently have an acceleration threshold so we are using 1 as a placeholder.
    long int value = 1;
    if (hook_get_pointer_sensitivity() < 0) {
        value = 0;
    }

    return value;
}

UIOHOOK_API long int hook_get_pointer_sensitivity() {
    #if defined(USE_APPLICATION_SERVICES) || defined(USE_IOKIT)
    bool successful = false;
    Float32 sensitivity;
    #endif

    long int value = -1;

    #ifdef USE_APPLICATION_SERVICES
    if (!successful) {
        CFTypeRef pref_val = CFPreferencesCopyValue(CFSTR("com.apple.mouse.scaling"), kCFPreferencesAnyApplication, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
        if (pref_val != NULL) {
            if (CFGetTypeID(pref_val) == CFNumberGetTypeID() && CFNumberGetValue((CFNumberRef) pref_val, kCFNumberFloat32Type, &sensitivity)) {
                value = (long) (sensitivity * MOUSE_ACCELERATION_MULTIPLIER);

                logger(LOG_LEVEL_DEBUG, "%s [%u]: CFPreferencesCopyValue com.apple.mouse.scaling: %li.\n",
                        __FUNCTION__, __LINE__, value);
            }

            CFRelease(pref_val);
        }
    }
    #endif

    #ifdef USE_IOKIT
    if (!successful) {
        CFTypeRef cf_type = NULL;
        kern_return_t kern_return = IOHIDCopyCFTypeParameter(connection, CFSTR(kIOHIDMouseAccelerationTypeKey), &cf_type);
        if (kern_return == kIOReturnSuccess) {
            if (cf_type != NULL) {
                if (CFGetTypeID(cf_type) == CFNumberGetTypeID()) {
                    if (CFNumberGetValue((CFNumberRef) cf_type, kCFNumberFloat32Type, &sensitivity)) {
                        value = (long) sensitivity;
                        successful = true;

                        logger(LOG_LEVEL_DEBUG, "%s [%u]: IOHIDCopyCFTypeParameter kIOHIDMouseAccelerationTypeKey: %li.\n",
                                __FUNCTION__, __LINE__, value);
                    }
                }

                CFRelease(cf_type);
            }
        }
    }
    #endif

    return value;
}

UIOHOOK_API long int hook_get_multi_click_time() {
    #if defined(USE_APPLICATION_SERVICES) || defined(USE_IOKIT) || defined(USE_CARBON_LEGACY)
    bool successful = false;
    Float64 time;
    #endif

    long int value = -1;

    #ifdef USE_APPLICATION_SERVICES
    if (!successful) {
        CFTypeRef pref_val = CFPreferencesCopyValue(CFSTR("com.apple.mouse.doubleClickThreshold"), kCFPreferencesAnyApplication, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
        if (pref_val != NULL) {
            if (CFGetTypeID(pref_val) == CFNumberGetTypeID() && CFNumberGetValue((CFNumberRef) pref_val, kCFNumberFloat64Type, &time)) {
                /* This appears to be the time in seconds */
                value = (long) (time * 1000);

                logger(LOG_LEVEL_DEBUG, "%s [%u]: CFPreferencesCopyValue: %li.\n",
                        __FUNCTION__, __LINE__, value);
            }

            CFRelease(pref_val);
        }
    }
    #endif

    #ifdef USE_IOKIT
    if (!successful) {
        CFTypeRef cf_type = NULL;
        kern_return_t kern_return = IOHIDCopyCFTypeParameter(connection, CFSTR(kIOHIDClickTimeKey), &cf_type);
        if (kern_return == kIOReturnSuccess) {
            if (cf_type != NULL) {
                if (CFGetTypeID(cf_type) == CFNumberGetTypeID()) {
                    if (CFNumberGetValue((CFNumberRef) cf_type, kCFNumberFloat64Type, &time)) {
                        /* This appears to be the time in nanoseconds */
                        value = (long) (time / 1000 / 1000);
                        successful = true;

                        logger(LOG_LEVEL_DEBUG, "%s [%u]: IORegistryEntryCreateCFProperty: %li.\n",
                                __FUNCTION__, __LINE__, value);
                    }
                }

                CFRelease(cf_type);
            }
        }
    }
    #endif

    #ifdef USE_CARBON_LEGACY
    if (!successful) {
        // Apple documentation states that value is in 'ticks'. I am not sure
        // what that means, but it looks a lot like the arbitrary slider value.
        time = GetDblTime();
        if (time > -1) {
            // This is the slider value, we must multiply by 15 to convert to
            // milliseconds.
            value = (long) time * 15;
            successful = true;

            logger(LOG_LEVEL_DEBUG, "%s [%u]: GetDblTime: %li.\n",
                    __FUNCTION__, __LINE__, value);
        }
    }
    #endif

    return value;
}


// Create a shared object constructor.
__attribute__ ((constructor))
void on_library_load() {
    #ifdef USE_IOKIT
    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching(kIOHIDSystemClass));
    if (service) {
        kern_return_t kren_ret = IOServiceOpen(service, mach_task_self(), kIOHIDParamConnectType, &connection);
        if (kren_ret != kIOReturnSuccess) {
            logger(LOG_LEVEL_INFO, "%s [%u]: IOServiceOpen failure (%#X)!\n",
                    __FUNCTION__, __LINE__, kren_ret);
        }
    }
    #endif
}

// Create a shared object destructor.
__attribute__ ((destructor))
void on_library_unload() {
    // Disable the event hook.
    //hook_stop();

    #ifdef USE_IOKIT
    if (connection) {
        kern_return_t kren_ret = IOServiceClose(connection);
        if (kren_ret != kIOReturnSuccess) {
            logger(LOG_LEVEL_INFO, "%s [%u]: IOServiceClose failure (%#X) %#X!\n",
                    __FUNCTION__, __LINE__, kren_ret, kIOReturnError);
        }
    }
    #endif
}
