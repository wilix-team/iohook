
#include "iohook.h"
#include "uiohook.h"
#include <iostream>

#if defined(__APPLE__) && defined(__MACH__)
#include <CoreFoundation/CoreFoundation.h>
#endif

using namespace v8;
using Callback = Nan::Callback;
static bool sIsRunning = false;
static bool sIsDebug = false;

static HookProcessWorker* sIOHook = nullptr;

static void dispatch_proc(uiohook_event * const event)
{
  if (sIOHook != nullptr && sIOHook->fHookExecution != nullptr)
  {
    sIOHook->fHookExecution->Send(event, sizeof(uiohook_event));
  }
}

static bool logger_proc(unsigned int level, const char *format, ...)
{
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

HookProcessWorker::HookProcessWorker(Nan::Callback * callback) :
Nan::AsyncProgressWorkerBase<uiohook_event>(callback),
fHookExecution(nullptr)
{

}

void HookProcessWorker::Execute(const Nan::AsyncProgressWorkerBase<uiohook_event>::ExecutionProgress& progress)
{
  hook_set_logger_proc(&logger_proc);
  hook_set_dispatch_proc(&dispatch_proc);
  fHookExecution = &progress;
  int status = hook_run();
  
  switch (status) {
    case UIOHOOK_SUCCESS:
      #if defined(__APPLE__) && defined(__MACH__)
      // NOTE Darwin requires that you start your own runloop from main.
      CFRunLoopRun();
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

void HookProcessWorker::Stop()
{
  hook_stop();
  sIsRunning = false;
}

void HookProcessWorker::HandleProgressCallback(const uiohook_event *event, size_t size)
{
  HandleScope scope(Isolate::GetCurrent());
  v8::Local<v8::Object> obj = Nan::New<v8::Object>();
  
  Nan::Set(obj, Nan::New("event").ToLocalChecked(), Nan::New(event));
  
  Nan::Set(obj, Nan::New("type").ToLocalChecked(), Nan::New((uint16_t)event->type));
  Nan::Set(obj, Nan::New("mask").ToLocalChecked(), Nan::New((uint16_t)event->mask));
  Nan::Set(obj, Nan::New("time").ToLocalChecked(), Nan::New((uint16_t)event->time));
  
  if ((event->type >= EVENT_KEY_TYPED) && (event->type <= EVENT_KEY_RELEASED)) {
    v8::Local<v8::Object> keyboard = Nan::New<v8::Object>();
    if (event->type == EVENT_KEY_TYPED) {
      Nan::Set(keyboard, Nan::New("keychar").ToLocalChecked(), Nan::New((uint16_t)event->data.keyboard.keychar));
    }
    Nan::Set(keyboard, Nan::New("keycode").ToLocalChecked(), Nan::New((uint16_t)event->data.keyboard.keycode));
    Nan::Set(keyboard, Nan::New("rawcode").ToLocalChecked(), Nan::New((uint16_t)event->data.keyboard.rawcode));
  
    Nan::Set(obj, Nan::New("keyboard").ToLocalChecked(), keyboard);
    v8::Local<v8::Value> argv[] = { obj };
    callback->Call(1, argv);
  } else if ((event->type >= EVENT_MOUSE_CLICKED) && (event->type < EVENT_MOUSE_WHEEL)) {
    v8::Local<v8::Object> mouse = Nan::New<v8::Object>();
    Nan::Set(mouse, Nan::New("button").ToLocalChecked(), Nan::New((uint16_t)event->data.mouse.button));
    Nan::Set(mouse, Nan::New("clicks").ToLocalChecked(), Nan::New((uint16_t)event->data.mouse.clicks));
    Nan::Set(mouse, Nan::New("x").ToLocalChecked(), Nan::New((int16_t)event->data.mouse.x));
    Nan::Set(mouse, Nan::New("y").ToLocalChecked(), Nan::New((int16_t)event->data.mouse.y));
    
    Nan::Set(obj, Nan::New("mouse").ToLocalChecked(), mouse);
    v8::Local<v8::Value> argv[] = { obj };
    callback->Call(1, argv);
  } else if (event->type == EVENT_MOUSE_WHEEL) {
    v8::Local<v8::Object> wheel = Nan::New<v8::Object>();
    Nan::Set(wheel, Nan::New("amount").ToLocalChecked(), Nan::New((uint16_t)event->data.wheel.amount));
    Nan::Set(wheel, Nan::New("clicks").ToLocalChecked(), Nan::New((uint16_t)event->data.wheel.clicks));
    Nan::Set(wheel, Nan::New("direction").ToLocalChecked(), Nan::New((int16_t)event->data.wheel.direction));
    Nan::Set(wheel, Nan::New("rotation").ToLocalChecked(), Nan::New((int16_t)event->data.wheel.rotation));
    Nan::Set(wheel, Nan::New("type").ToLocalChecked(), Nan::New((int16_t)event->data.wheel.type));
    Nan::Set(wheel, Nan::New("x").ToLocalChecked(), Nan::New((int16_t)event->data.wheel.x));
    Nan::Set(wheel, Nan::New("y").ToLocalChecked(), Nan::New((int16_t)event->data.wheel.y));
  
    Nan::Set(obj, Nan::New("wheel").ToLocalChecked(), wheel);
    v8::Local<v8::Value> argv[] = { obj };
    callback->Call(1, argv);
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
}

NODE_MODULE(nodeHook, Init)