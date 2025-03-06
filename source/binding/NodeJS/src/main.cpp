#include "include/info.h"
#include "include/loader.h"
#include "include/utils.h"

#include <MaaFramework/MaaAPI.h>
#include <napi.h>

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    auto extCtx = Napi::External<ExtContextInfo>::New(env, new ExtContextInfo, &DeleteFinalizer<ExtContextInfo*>);
    exports["__context"] = extCtx;

    load_instance_context(env, exports, extCtx);
    load_instance_controller(env, exports, extCtx);
    load_instance_resource(env, exports, extCtx);
    load_instance_tasker(env, exports, extCtx);
    load_utility_utility(env, exports, extCtx);

    load_toolkit_config(env, exports, extCtx);
    load_toolkit_find(env, exports, extCtx);
    load_toolkit_pi(env, exports, extCtx);

    load_agent(env, exports, extCtx);

#define DE(prefix, key) prefix##_obj[#key] = JSConvert<prefix>::to_value(env, prefix##_##key)

    auto MaaStatus_obj = Napi::Object::New(env);
    DE(MaaStatus, Invalid);
    DE(MaaStatus, Pending);
    DE(MaaStatus, Running);
    DE(MaaStatus, Succeeded);
    DE(MaaStatus, Failed);
    exports["Status"] = MaaStatus_obj;

    auto MaaLoggingLevel_obj = Napi::Object::New(env);
    DE(MaaLoggingLevel, Off);
    DE(MaaLoggingLevel, Fatal);
    DE(MaaLoggingLevel, Error);
    DE(MaaLoggingLevel, Warn);
    DE(MaaLoggingLevel, Info);
    DE(MaaLoggingLevel, Debug);
    DE(MaaLoggingLevel, Trace);
    DE(MaaLoggingLevel, All);
    exports["LoggingLevel"] = MaaLoggingLevel_obj;

    auto MaaAdbScreencapMethod_obj = Napi::Object::New(env);
    DE(MaaAdbScreencapMethod, EncodeToFileAndPull);
    DE(MaaAdbScreencapMethod, Encode);
    DE(MaaAdbScreencapMethod, RawWithGzip);
    DE(MaaAdbScreencapMethod, RawByNetcat);
    DE(MaaAdbScreencapMethod, MinicapDirect);
    DE(MaaAdbScreencapMethod, MinicapStream);
    DE(MaaAdbScreencapMethod, EmulatorExtras);
    DE(MaaAdbScreencapMethod, All);
    DE(MaaAdbScreencapMethod, Default);
    exports["AdbScreencapMethod"] = MaaAdbScreencapMethod_obj;

    auto MaaAdbInputMethod_obj = Napi::Object::New(env);
    DE(MaaAdbInputMethod, AdbShell);
    DE(MaaAdbInputMethod, MinitouchAndAdbKey);
    DE(MaaAdbInputMethod, Maatouch);
    DE(MaaAdbInputMethod, EmulatorExtras);
    DE(MaaAdbInputMethod, All);
    DE(MaaAdbInputMethod, Default);
    exports["AdbInputMethod"] = MaaAdbInputMethod_obj;

    auto MaaWin32ScreencapMethod_obj = Napi::Object::New(env);
    DE(MaaWin32ScreencapMethod, GDI);
    DE(MaaWin32ScreencapMethod, FramePool);
    DE(MaaWin32ScreencapMethod, DXGI_DesktopDup);
    exports["Win32ScreencapMethod"] = MaaWin32ScreencapMethod_obj;

    auto MaaWin32InputMethod_obj = Napi::Object::New(env);
    DE(MaaWin32InputMethod, Seize);
    DE(MaaWin32InputMethod, SendMessage);
    exports["Win32InputMethod"] = MaaWin32InputMethod_obj;

    auto MaaDbgControllerType_obj = Napi::Object::New(env);
    DE(MaaDbgControllerType, CarouselImage);
    DE(MaaDbgControllerType, ReplayRecording);
    exports["DbgControllerType"] = MaaDbgControllerType_obj;

    auto MaaInferenceDevice_obj = Napi::Object::New(env);
    DE(MaaInferenceDevice, CPU);
    DE(MaaInferenceDevice, Auto);
    exports["InferenceDevice"] = MaaInferenceDevice_obj;

    auto MaaInferenceExecutionProvider_obj = Napi::Object::New(env);
    DE(MaaInferenceExecutionProvider, Auto);
    DE(MaaInferenceExecutionProvider, CPU);
    DE(MaaInferenceExecutionProvider, DirectML);
    DE(MaaInferenceExecutionProvider, CoreML);
    DE(MaaInferenceExecutionProvider, CUDA);
    exports["InferenceExecutionProvider"] = MaaInferenceExecutionProvider_obj;

    return exports;
}

NODE_API_MODULE(maa, Init)
