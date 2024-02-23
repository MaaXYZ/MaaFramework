#include <cstring>

#include "MaaFramework/Instance/MaaInstance.h"
#include "MaaFramework/MaaDef.h"
#include "extra.h"

MaaBool MaaControllerSetOptionString(MaaControllerHandle ctrl, MaaCtrlOption key, MaaStringView value)
{
    return MaaControllerSetOption(ctrl, key, const_cast<char*>(value), strlen(value));
}

MaaBool MaaControllerSetOptionInteger(MaaControllerHandle ctrl, MaaCtrlOption key, int value)
{
    return MaaControllerSetOption(ctrl, key, &value, 4);
}

MaaBool MaaControllerSetOptionBoolean(MaaControllerHandle ctrl, MaaCtrlOption key, bool value)
{
    MaaBool v = !!value;
    return MaaControllerSetOption(ctrl, key, &v, 1);
}

MaaBool MaaSetGlobalOptionString(MaaCtrlOption key, MaaStringView value)
{
    return MaaSetGlobalOption(key, const_cast<char*>(value), strlen(value));
}

MaaBool MaaSetGlobalOptionInteger(MaaCtrlOption key, int value)
{
    return MaaSetGlobalOption(key, &value, 4);
}

MaaBool MaaSetGlobalOptionBoolean(MaaCtrlOption key, bool value)
{
    MaaBool v = !!value;
    return MaaSetGlobalOption(key, &v, 1);
}

struct CustomRecoInfo {
    MaaCustomRecognizerAPI api;
    MaaTransparentArg analyze;
};

MaaBool FAKE_IMPORT MaaRegisterCustomRecognizerImpl(MaaInstanceHandle inst, MaaStringView name, decltype(MaaCustomRecognizerAPI{}.analyze) analyze, MaaTransparentArg analyze_arg) {
    static MaaCustomRecognizerAPI api = {
        [](auto sycctx, auto img, auto task, auto param, auto ctx, auto out_box, auto out_detail) {
            auto info = reinterpret_cast<CustomRecoInfo *>(ctx);
            return info->api.analyze(sycctx, img, task, param, info->analyze, out_box, out_detail);
        }
    };
    auto info = new CustomRecoInfo { {analyze}, analyze_arg };
    return MaaRegisterCustomRecognizer(inst, name, &api, info);
}

struct CustomActionInfo
{
    MaaCustomActionAPI api;
    MaaTransparentArg run;
    MaaTransparentArg stop;
};

MaaBool FAKE_IMPORT MaaRegisterCustomActionImpl(MaaInstanceHandle inst, MaaStringView name,
                                                decltype(MaaCustomActionAPI {}.run) run, MaaTransparentArg run_arg,
                                                decltype(MaaCustomActionAPI {}.stop) stop, MaaTransparentArg stop_arg)
{
    static MaaCustomActionAPI api = {
        [](auto sycctx, auto task, auto param, auto box, auto detail, auto ctx) {
            auto info = reinterpret_cast<CustomActionInfo *>(ctx);
            return info->api.run(sycctx, task, param, box, detail, info->run);
        },
        [](auto ctx) {
            auto info = reinterpret_cast<CustomActionInfo *>(ctx);
            info->api.stop(info->stop);
        }
    };
    auto info = new CustomActionInfo { { run, stop }, run_arg, stop_arg };

    return MaaRegisterCustomAction(inst, name, &api, info);
}
