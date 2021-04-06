#include "iohook.h"
#include "uiohook.h"

#ifdef _WIN32
#include <windows.h>
#else
#if defined(__APPLE__) && defined(__MACH__)
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <pthread.h>
#endif
#include <queue>

using namespace v8;
using Callback = Nan::Callback;
static bool sIsRunning = false;
static bool sIsDebug = false;

static HookProcessWorker* sIOHook = nullptr;

static std::queue<uiohook_event> zqueue;

// Native thread errors.
#define UIOHOOK_ERROR_THREAD_CREATE       0x10

// Thread and mutex variables.
#ifdef _WIN32
static HANDLE hook_thread;

static CRITICAL_SECTION hook_running_mutex;
static CRITICAL_SECTION hook_control_mutex;
static CONDITION_VARIABLE hook_control_cond;
#else
static pthread_t hook_thread;

static pthread_mutex_t hook_running_mutex;
static pthread_mutex_t hook_control_mutex;
static pthread_cond_t hook_control_cond;
#endif

bool logger_proc(unsigned int level, const char *format, ...) {
  if (!sIsDebug) {
    return false;
  }
  bool status = false;
  va_list args;
  switch (level) {
    case LOG_LEVEL_DEBUG:
    case LOG_LEVEL_INFO:
      va_start(args, format);
      status = vfprintf(stdout, format, args) >= 0;
      va_end(args);
      break;

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
  switch (event->type) {
    case EVENT_HOOK_ENABLED:
      // Lock the running mutex so we know if the hook is enabled.
      #ifdef _WIN32
      EnterCriticalSection(&hook_running_mutex);
      #else
      pthread_mutex_lock(&hook_running_mutex);
      #endif

      // Unlock the control mutex so hook_enable() can continue.
      #ifdef _WIN32
      WakeConditionVariable(&hook_control_cond);
      LeaveCriticalSection(&hook_control_mutex);
      #else
      // Unlock the control mutex so hook_enable() can continue.
      pthread_cond_signal(&hook_control_cond);
      pthread_mutex_unlock(&hook_control_mutex);
      #endif
      break;

    case EVENT_HOOK_DISABLED:
      // Lock the control mutex until we exit.
      #ifdef _WIN32
      EnterCriticalSection(&hook_control_mutex);
      #else
      pthread_mutex_lock(&hook_control_mutex);
      #endif

      // Unlock the running mutex so we know if the hook is disabled.
      #ifdef _WIN32
      LeaveCriticalSection(&hook_running_mutex);
      #else
      #if defined(__APPLE__) && defined(__MACH__)
      // Stop the main runloop so that this program ends.
      CFRunLoopStop(CFRunLoopGetMain());
      #endif

      pthread_mutex_unlock(&hook_running_mutex);
      #endif
      break;

    case EVENT_KEY_PRESSED:
    case EVENT_KEY_RELEASED:
    case EVENT_KEY_TYPED:
    case EVENT_MOUSE_PRESSED:
    case EVENT_MOUSE_RELEASED:
    case EVENT_MOUSE_CLICKED:
    case EVENT_MOUSE_MOVED:
    case EVENT_MOUSE_DRAGGED:
    case EVENT_MOUSE_WHEEL:
      uiohook_event event_copy;
      memcpy(&event_copy, event, sizeof(uiohook_event));
      zqueue.push(event_copy);
      sIOHook->fHookExecution->Send(event, sizeof(uiohook_event));
      break;
  }
}

#ifdef _WIN32
DWORD WINAPI hook_thread_proc(LPVOID arg) {
#else
void *hook_thread_proc(void *arg) {
#endif
  // Set the hook status.
  int status = hook_run();
  if (status != UIOHOOK_SUCCESS) {
    #ifdef _WIN32
    *(DWORD *) arg = status;
    #else
    *(int *) arg = status;
    #endif
  }

  // Make sure we signal that we have passed any exception throwing code for
  // the waiting hook_enable().
  #ifdef _WIN32
  WakeConditionVariable(&hook_control_cond);
  LeaveCriticalSection(&hook_control_mutex);

  return status;
  #else
  // Make sure we signal that we have passed any exception throwing code for
  // the waiting hook_enable().
  pthread_cond_signal(&hook_control_cond);
  pthread_mutex_unlock(&hook_control_mutex);

  return arg;
  #endif
}

int hook_enable() {
  // Lock the thread control mutex.  This will be unlocked when the
  // thread has finished starting, or when it has fully stopped.
  #ifdef _WIN32
  EnterCriticalSection(&hook_control_mutex);
  #else
  pthread_mutex_lock(&hook_control_mutex);
  #endif

  // Set the initial status.
  int status = UIOHOOK_FAILURE;

  #ifndef _WIN32
  // Create the thread attribute.
  pthread_attr_t hook_thread_attr;
  pthread_attr_init(&hook_thread_attr);

  // Get the policy and priority for the thread attr.
  int policy;
  pthread_attr_getschedpolicy(&hook_thread_attr, &policy);
  int priority = sched_get_priority_max(policy);
  #endif

  #if defined(_WIN32)
  DWORD hook_thread_id;
  DWORD *hook_thread_status = (DWORD *) malloc(sizeof(DWORD));
  hook_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) hook_thread_proc, hook_thread_status, 0, &hook_thread_id);
  if (hook_thread != INVALID_HANDLE_VALUE) {
  #else
  int *hook_thread_status = (int*)malloc(sizeof(int));
  if (pthread_create(&hook_thread, &hook_thread_attr, hook_thread_proc, hook_thread_status) == 0) {
  #endif
    #if defined(_WIN32)
    // Attempt to set the thread priority to time critical.
    if (SetThreadPriority(hook_thread, THREAD_PRIORITY_TIME_CRITICAL) == 0) {
      logger_proc(LOG_LEVEL_WARN, "%s [%u]: Could not set thread priority %li for thread %#p! (%#lX)\n",
          __FUNCTION__, __LINE__, (long) THREAD_PRIORITY_TIME_CRITICAL,
          hook_thread, (unsigned long) GetLastError());
    }
    #elif (defined(__APPLE__) && defined(__MACH__)) || _POSIX_C_SOURCE >= 200112L
    // Some POSIX revisions do not support pthread_setschedprio so we will
    // use pthread_setschedparam instead.
    struct sched_param param = { .sched_priority = priority };
    if (pthread_setschedparam(hook_thread, SCHED_OTHER, &param) != 0) {
      logger_proc(LOG_LEVEL_WARN, "%s [%u]: Could not set thread priority %i for thread 0x%lX!\n",
          __FUNCTION__, __LINE__, priority, (unsigned long) hook_thread);
    }
    #else
    // Raise the thread priority using glibc pthread_setschedprio.
    if (pthread_setschedprio(hook_thread, priority) != 0) {
      logger_proc(LOG_LEVEL_WARN, "%s [%u]: Could not set thread priority %i for thread 0x%lX!\n",
          __FUNCTION__, __LINE__, priority, (unsigned long) hook_thread);
    }
    #endif


    // Wait for the thread to indicate that it has passed the
    // initialization portion by blocking until either a EVENT_HOOK_ENABLED
    // event is received or the thread terminates.
    // NOTE This unlocks the hook_control_mutex while we wait.
    #ifdef _WIN32
    SleepConditionVariableCS(&hook_control_cond, &hook_control_mutex, INFINITE);
    #else
    pthread_cond_wait(&hook_control_cond, &hook_control_mutex);
    #endif

    #ifdef _WIN32
    if (TryEnterCriticalSection(&hook_running_mutex) != FALSE) {
    #else
    if (pthread_mutex_trylock(&hook_running_mutex) == 0) {
    #endif
      // Lock Successful; The hook is not running but the hook_control_cond
      // was signaled!  This indicates that there was a startup problem!

      // Get the status back from the thread.
      #ifdef _WIN32
      WaitForSingleObject(hook_thread,  INFINITE);
      GetExitCodeThread(hook_thread, hook_thread_status);
      #else
      pthread_join(hook_thread, (void **) &hook_thread_status);
      status = *hook_thread_status;
      #endif
    }
    else {
      // Lock Failure; The hook is currently running and wait was signaled
      // indicating that we have passed all possible start checks.  We can
      // always assume a successful startup at this point.
      status = UIOHOOK_SUCCESS;
    }

    free(hook_thread_status);

    logger_proc(LOG_LEVEL_DEBUG,  "%s [%u]: Thread Result: (%#X).\n",
        __FUNCTION__, __LINE__, status);
  }
  else {
    status = UIOHOOK_ERROR_THREAD_CREATE;
  }

  // Make sure the control mutex is unlocked.
  #ifdef _WIN32
  LeaveCriticalSection(&hook_control_mutex);
  #else
  pthread_mutex_unlock(&hook_control_mutex);
  #endif

  return status;
}

void run() {
  // Lock the thread control mutex.  This will be unlocked when the
  // thread has finished starting, or when it has fully stopped.
  #ifdef _WIN32
  // Create event handles for the thread hook.
  InitializeCriticalSection(&hook_running_mutex);
  InitializeCriticalSection(&hook_control_mutex);
  InitializeConditionVariable(&hook_control_cond);
  #else
  pthread_mutex_init(&hook_running_mutex, NULL);
  pthread_mutex_init(&hook_control_mutex, NULL);
  pthread_cond_init(&hook_control_cond, NULL);
  #endif

  // Set the logger callback for library output.
  hook_set_logger_proc(&logger_proc);

  // Set the event callback for uiohook events.
  hook_set_dispatch_proc(&dispatch_proc);

  // Start the hook and block.
  // NOTE If EVENT_HOOK_ENABLED was delivered, the status will always succeed.
  int status = hook_enable();
  switch (status) {
    case UIOHOOK_SUCCESS:
      // We no longer block, so we need to explicitly wait for the thread to die.
      #ifdef _WIN32
      WaitForSingleObject(hook_thread,  INFINITE);
      #else
      #if defined(__APPLE__) && defined(__MACH__)
      // NOTE Darwin requires that you start your own runloop from main.
      CFRunLoopRun();
      #endif

      pthread_join(hook_thread, NULL);
      #endif
      break;

    // System level errors.
    case UIOHOOK_ERROR_OUT_OF_MEMORY:
      logger_proc(LOG_LEVEL_ERROR, "Failed to allocate memory. (%#X)\n", status);
      break;


    // X11 specific errors.
    case UIOHOOK_ERROR_X_OPEN_DISPLAY:
      logger_proc(LOG_LEVEL_ERROR, "Failed to open X11 display. (%#X)\n", status);
      break;

    case UIOHOOK_ERROR_X_RECORD_NOT_FOUND:
      logger_proc(LOG_LEVEL_ERROR, "Unable to locate XRecord extension. (%#X)\n", status);
      break;

    case UIOHOOK_ERROR_X_RECORD_ALLOC_RANGE:
      logger_proc(LOG_LEVEL_ERROR, "Unable to allocate XRecord range. (%#X)\n", status);
      break;

    case UIOHOOK_ERROR_X_RECORD_CREATE_CONTEXT:
      logger_proc(LOG_LEVEL_ERROR, "Unable to allocate XRecord context. (%#X)\n", status);
      break;

    case UIOHOOK_ERROR_X_RECORD_ENABLE_CONTEXT:
      logger_proc(LOG_LEVEL_ERROR, "Failed to enable XRecord context. (%#X)\n", status);
      break;


    // Windows specific errors.
    case UIOHOOK_ERROR_SET_WINDOWS_HOOK_EX:
      logger_proc(LOG_LEVEL_ERROR, "Failed to register low level windows hook. (%#X)\n", status);
      break;


    // Darwin specific errors.
    case UIOHOOK_ERROR_AXAPI_DISABLED:
      logger_proc(LOG_LEVEL_ERROR, "Failed to enable access for assistive devices. (%#X)\n", status);
      break;

    case UIOHOOK_ERROR_CREATE_EVENT_PORT:
      logger_proc(LOG_LEVEL_ERROR, "Failed to create apple event port. (%#X)\n", status);
      break;

    case UIOHOOK_ERROR_CREATE_RUN_LOOP_SOURCE:
      logger_proc(LOG_LEVEL_ERROR, "Failed to create apple run loop source. (%#X)\n", status);
      break;

    case UIOHOOK_ERROR_GET_RUNLOOP:
      logger_proc(LOG_LEVEL_ERROR, "Failed to acquire apple run loop. (%#X)\n", status);
      break;

    case UIOHOOK_ERROR_CREATE_OBSERVER:
      logger_proc(LOG_LEVEL_ERROR, "Failed to create apple run loop observer. (%#X)\n", status);
      break;

    // Default error.
    case UIOHOOK_FAILURE:
    default:
      logger_proc(LOG_LEVEL_ERROR, "An unknown hook error occurred. (%#X)\n", status);
      break;
  }
}

void stop() {
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

  #ifdef _WIN32
  // Create event handles for the thread hook.
  CloseHandle(hook_thread);
  DeleteCriticalSection(&hook_running_mutex);
  DeleteCriticalSection(&hook_control_mutex);
  #else
  pthread_mutex_destroy(&hook_running_mutex);
  pthread_mutex_destroy(&hook_control_mutex);
  pthread_cond_destroy(&hook_control_cond);
  #endif
}

HookProcessWorker::HookProcessWorker(Nan::Callback * callback) :
Nan::AsyncProgressWorkerBase<uiohook_event>(callback),
fHookExecution(nullptr)
{

}

v8::Local<v8::Object> fillEventObject(uiohook_event event) {
  v8::Local<v8::Object> obj = Nan::New<v8::Object>();

  obj->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("type").ToLocalChecked(), Nan::New((uint16_t)event.type));
  obj->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("mask").ToLocalChecked(), Nan::New((uint16_t)event.mask));
  obj->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("time").ToLocalChecked(), Nan::New((uint16_t)event.time));

  if ((event.type >= EVENT_KEY_TYPED) && (event.type <= EVENT_KEY_RELEASED)) {
    v8::Local<v8::Object> keyboard = Nan::New<v8::Object>();

    if (event.data.keyboard.keycode == VC_SHIFT_L || event.data.keyboard.keycode == VC_SHIFT_R) {
      keyboard->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("shiftKey").ToLocalChecked(), Nan::New(true));
    } else {
      keyboard->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("shiftKey").ToLocalChecked(), Nan::New(false));
    }

    if (event.data.keyboard.keycode == VC_ALT_L || event.data.keyboard.keycode == VC_ALT_R) {
      keyboard->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("altKey").ToLocalChecked(), Nan::New(true));
    } else {
      keyboard->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("altKey").ToLocalChecked(), Nan::New(false));
    }

    if (event.data.keyboard.keycode == VC_CONTROL_L || event.data.keyboard.keycode == VC_CONTROL_R) {
      keyboard->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("ctrlKey").ToLocalChecked(), Nan::New(true));
    } else {
      keyboard->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("ctrlKey").ToLocalChecked(), Nan::New(false));
    }

    if (event.data.keyboard.keycode == VC_META_L || event.data.keyboard.keycode == VC_META_R) {
      keyboard->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("metaKey").ToLocalChecked(), Nan::New(true));
    } else {
      keyboard->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("metaKey").ToLocalChecked(), Nan::New(false));
    }

    if (event.type == EVENT_KEY_TYPED) {
      keyboard->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("keychar").ToLocalChecked(), Nan::New((uint16_t)event.data.keyboard.keychar));
    }

    keyboard->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("keycode").ToLocalChecked(), Nan::New((uint16_t)event.data.keyboard.keycode));
    keyboard->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("rawcode").ToLocalChecked(), Nan::New((uint16_t)event.data.keyboard.rawcode));

    obj->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("keyboard").ToLocalChecked(), keyboard);
  } else if ((event.type >= EVENT_MOUSE_CLICKED) && (event.type < EVENT_MOUSE_WHEEL)) {
    v8::Local<v8::Object> mouse = Nan::New<v8::Object>();
    mouse->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("button").ToLocalChecked(), Nan::New((uint16_t)event.data.mouse.button));
    mouse->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("clicks").ToLocalChecked(), Nan::New((uint16_t)event.data.mouse.clicks));
    mouse->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("x").ToLocalChecked(), Nan::New((int16_t)event.data.mouse.x));
    mouse->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("y").ToLocalChecked(), Nan::New((int16_t)event.data.mouse.y));

    obj->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("mouse").ToLocalChecked(), mouse);
  } else if (event.type == EVENT_MOUSE_WHEEL) {
    v8::Local<v8::Object> wheel = Nan::New<v8::Object>();
    wheel->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("amount").ToLocalChecked(), Nan::New((uint16_t)event.data.wheel.amount));
    wheel->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("clicks").ToLocalChecked(), Nan::New((uint16_t)event.data.wheel.clicks));
    wheel->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("direction").ToLocalChecked(), Nan::New((int16_t)event.data.wheel.direction));
    wheel->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("rotation").ToLocalChecked(), Nan::New((int16_t)event.data.wheel.rotation));
    wheel->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("type").ToLocalChecked(), Nan::New((int16_t)event.data.wheel.type));
    wheel->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("x").ToLocalChecked(), Nan::New((int16_t)event.data.wheel.x));
    wheel->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("y").ToLocalChecked(), Nan::New((int16_t)event.data.wheel.y));

    obj->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Nan::New("wheel").ToLocalChecked(), wheel);
  }
  return obj;
}

void HookProcessWorker::HandleProgressCallback(const uiohook_event * event, size_t size)
{
  uiohook_event ev;
  while (!zqueue.empty()) {
    ev = zqueue.front();

    HandleScope scope(Isolate::GetCurrent());

    v8::Local<v8::Object> obj = fillEventObject(ev);

    v8::Local<v8::Value> argv[] = { obj };
    callback->Call(1, argv);

    zqueue.pop();
  }
}

void HookProcessWorker::Execute(const Nan::AsyncProgressWorkerBase<uiohook_event>::ExecutionProgress& progress)
{
  fHookExecution = &progress;
  run();
}

void HookProcessWorker::Stop()
{
  stop();
  sIsRunning = false;
}

NAN_METHOD(GrabMouseClick) {
  if (info.Length() > 0)
  {
    grab_mouse_click(info[0]->IsTrue());
  }
}

NAN_METHOD(DebugEnable) {
  if (info.Length() > 0)
  {
    sIsDebug = info[0]->IsTrue();
  }
}

NAN_METHOD(StartHook) {
  //allow one single execution
  if (sIsRunning == false)
  {
    if (info.Length() > 0)
    {
      if (info.Length() == 2) {
        if (info[1]->IsTrue()) {
          sIsDebug = true;
        } else {
          sIsDebug = false;
        }
      }
      if (info[0]->IsFunction())
      {
        Callback* callback = new Callback(info[0].As<Function>());
        sIOHook = new HookProcessWorker(callback);
        Nan::AsyncQueueWorker(sIOHook);
        sIsRunning = true;
      }
    }
  }
}

NAN_METHOD(StopHook) {
  //allow one single execution
  if ((sIsRunning == true) && (sIOHook != nullptr))
  {
    sIOHook->Stop();
  }
}

NAN_MODULE_INIT(Init) {
  Nan::Set(target, Nan::New<String>("startHook").ToLocalChecked(),
  Nan::GetFunction(Nan::New<FunctionTemplate>(StartHook)).ToLocalChecked());

  Nan::Set(target, Nan::New<String>("stopHook").ToLocalChecked(),
  Nan::GetFunction(Nan::New<FunctionTemplate>(StopHook)).ToLocalChecked());

  Nan::Set(target, Nan::New<String>("debugEnable").ToLocalChecked(),
  Nan::GetFunction(Nan::New<FunctionTemplate>(DebugEnable)).ToLocalChecked());

  Nan::Set(target, Nan::New<String>("grabMouseClick").ToLocalChecked(),
  Nan::GetFunction(Nan::New<FunctionTemplate>(GrabMouseClick)).ToLocalChecked());
}

NODE_MODULE(nodeHook, Init)
