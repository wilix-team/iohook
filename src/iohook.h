#pragma once

#include <napi.h>

#include "uiohook.h"

class HookProcessWorker : public Napi::AsyncProgressWorker<uiohook_event>
{
public:
    HookProcessWorker(const Napi::Function& callback);
    
protected:
    void Execute(const ExecutionProgress& progress) override;
    void OnProgress(const uiohook_event* event, size_t size) override;

public:
    void Send(const uiohook_event* event, size_t size);
    void Stop();
private:
    const ExecutionProgress* fHookExecution;
};
