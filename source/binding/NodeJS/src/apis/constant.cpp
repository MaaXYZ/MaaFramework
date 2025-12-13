#include "loader.h"

#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"

#define DE(prefix, key) obj[#key] = maajs::JSConvert<prefix>::to_value(env, prefix##Enum::prefix##_##key)
#define DEM(prefix, key) obj[#key] = maajs::JSConvert<prefix>::to_value(env, prefix##_##key)

static maajs::ValueType load_status(maajs::EnvType env)
{
    auto obj = maajs::ObjectType::New(env);

    DE(MaaStatus, Invalid);
    DE(MaaStatus, Pending);
    DE(MaaStatus, Running);
    DE(MaaStatus, Succeeded);
    DE(MaaStatus, Failed);

    return obj;
}

static maajs::ValueType load_adb_screencap_method(maajs::EnvType env)
{
    auto obj = maajs::ObjectType::New(env);

    DEM(MaaAdbScreencapMethod, EncodeToFileAndPull);
    DEM(MaaAdbScreencapMethod, Encode);
    DEM(MaaAdbScreencapMethod, RawWithGzip);
    DEM(MaaAdbScreencapMethod, RawByNetcat);
    DEM(MaaAdbScreencapMethod, MinicapDirect);
    DEM(MaaAdbScreencapMethod, MinicapStream);
    DEM(MaaAdbScreencapMethod, EmulatorExtras);
    DEM(MaaAdbScreencapMethod, All);
    DEM(MaaAdbScreencapMethod, Default);

    return obj;
}

static maajs::ValueType load_adb_input_method(maajs::EnvType env)
{
    auto obj = maajs::ObjectType::New(env);

    DEM(MaaAdbInputMethod, AdbShell);
    DEM(MaaAdbInputMethod, MinitouchAndAdbKey);
    DEM(MaaAdbInputMethod, Maatouch);
    DEM(MaaAdbInputMethod, EmulatorExtras);
    DEM(MaaAdbInputMethod, All);
    DEM(MaaAdbInputMethod, Default);

    return obj;
}

static maajs::ValueType load_win32_screencap_method(maajs::EnvType env)
{
    auto obj = maajs::ObjectType::New(env);

    DEM(MaaWin32ScreencapMethod, GDI);
    DEM(MaaWin32ScreencapMethod, FramePool);
    DEM(MaaWin32ScreencapMethod, DXGI_DesktopDup);
    DEM(MaaWin32ScreencapMethod, DXGI_DesktopDup_Window);
    DEM(MaaWin32ScreencapMethod, PrintWindow);
    DEM(MaaWin32ScreencapMethod, ScreenDC);

    return obj;
}

static maajs::ValueType load_win32_input_method(maajs::EnvType env)
{
    auto obj = maajs::ObjectType::New(env);

    DEM(MaaWin32InputMethod, Seize);
    DEM(MaaWin32InputMethod, SendMessage);
    DEM(MaaWin32InputMethod, PostMessage);
    DEM(MaaWin32InputMethod, LegacyEvent);
    DEM(MaaWin32InputMethod, PostThreadMessage);
    DEM(MaaWin32InputMethod, SendMessageWithCursorPos);
    DEM(MaaWin32InputMethod, PostMessageWithCursorPos);
    DEM(MaaWin32InputMethod, Gamepad);

    return obj;
}

static maajs::ValueType load_dbg_controller_type(maajs::EnvType env)
{
    auto obj = maajs::ObjectType::New(env);

    DEM(MaaDbgControllerType, CarouselImage);
    DEM(MaaDbgControllerType, ReplayRecording);

    return obj;
}

#define DS(key) obj[#key] = maajs::StringType::New(env, #key)

static maajs::ValueType load_recognition_type(maajs::EnvType env)
{
    auto obj = maajs::ObjectType::New(env);

    DS(DirectHit);
    DS(TemplateMatch);
    DS(FeatureMatch);
    DS(ColorMatch);
    DS(OCR);
    DS(NeuralNetworkClassify);
    DS(NeuralNetworkDetect);
    DS(Custom);

    return obj;
}

static maajs::ValueType load_action_type(maajs::EnvType env)
{
    auto obj = maajs::ObjectType::New(env);

    DS(DoNothing);
    DS(Click);
    DS(LongPress);
    DS(Swipe);
    DS(MultiSwipe);
    DS(TouchDown);
    DS(TouchMove);
    DS(TouchUp);
    DS(ClickKey);
    DS(LongPressKey);
    DS(KeyDown);
    DS(KeyUp);
    DS(InputText);
    DS(StartApp);
    DS(StopApp);
    DS(StopTask);
    DS(Scroll);
    DS(Command);
    DS(Custom);

    return obj;
}

#undef DS

std::map<std::string, maajs::ValueType> load_constant(maajs::EnvType env)
{
    return {
        { "Status", load_status(env) },
        { "AdbScreencapMethod", load_adb_screencap_method(env) },
        { "AdbInputMethod", load_adb_input_method(env) },
        { "Win32ScreencapMethod", load_win32_screencap_method(env) },
        { "Win32InputMethod", load_win32_input_method(env) },
        { "DbgControllerType", load_dbg_controller_type(env) },
        { "RecognitionType", load_recognition_type(env) },
        { "ActionType", load_action_type(env) },
    };
}
