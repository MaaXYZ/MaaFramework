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

    return obj;
}

static maajs::ValueType load_win32_input_method(maajs::EnvType env)
{
    auto obj = maajs::ObjectType::New(env);

    DEM(MaaWin32InputMethod, Seize);
    DEM(MaaWin32InputMethod, SendMessage);

    return obj;
}

std::map<std::string, maajs::ValueType> load_constant(maajs::EnvType env)
{
    return {
        { "Status", load_status(env) },
        { "AdbScreencapMethod", load_adb_screencap_method(env) },
        { "AdbInputMethod", load_adb_input_method(env) },
        { "Win32ScreencapMethod", load_win32_screencap_method(env) },
        { "Win32InputMethod", load_win32_input_method(env) },
    };
}
