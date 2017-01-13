#pragma once

#include <nan.h>

#include <nan_object_wrap.h>

#include "uiohook.h"

class HookProcessWorker : public Nan::AsyncProgressWorkerBase<uiohook_event>
{
  public:
  
    typedef Nan::AsyncProgressWorkerBase<uiohook_event>::ExecutionProgress HookExecution;
  
    HookProcessWorker(Nan::Callback * callback);
  
    void Execute(const ExecutionProgress& progress);
  
    void HandleProgressCallback(const uiohook_event *event, size_t size);
  
    void Stop();
  
    const HookExecution* fHookExecution;
};