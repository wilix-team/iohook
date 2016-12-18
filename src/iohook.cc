
#include "iohook.h"
#include "uiohook.h"
#include <iostream>

using namespace v8;
using Callback = Nan::Callback;
static bool sIsRunning = false; 

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
  return true;
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
  hook_run();
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
  if(sIsRunning == false)
  {
    if(info.Length() > 0)
    {
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