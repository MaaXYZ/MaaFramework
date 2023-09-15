#include <MaaFramework/MaaAPI.h>
#include <MaaFramework/MaaCustomController.h>
#include <stddef.h>

using MaaJSCustomControllerHandle = MaaBool (*)(MaaStringView cmd, size_t* args);

static MaaBool __set_option(MaaCallbackTransparentArg* ptr, MaaCtrlOption key, MaaStringView value)
{
    size_t arr[] = { (size_t)&key, (size_t)value };
    return reinterpret_cast<MaaJSCustomControllerHandle>(ptr)("set_option", arr);
}

static MaaBool __connect(MaaCallbackTransparentArg* ptr)
{
    return reinterpret_cast<MaaJSCustomControllerHandle>(ptr)("connect", nullptr);
}

static MaaBool __click(MaaCallbackTransparentArg* ptr, int32_t x, int32_t y)
{
    size_t arr[] = { (size_t)&x, (size_t)&y };
    return reinterpret_cast<MaaJSCustomControllerHandle>(ptr)("click", arr);
}

static MaaBool __swipe(MaaCallbackTransparentArg* ptr, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration)
{
    size_t arr[] = { (size_t)&x1, (size_t)&y1, (size_t)&x2, (size_t)&y2, (size_t)&duration };
    return reinterpret_cast<MaaJSCustomControllerHandle>(ptr)("swipe", arr);
}

static MaaCustomControllerAPI __custom_controller_api {
    __set_option,
    __connect,
    __click,
    __swipe,
};

extern "C"
{
    MaaControllerHandle MAA_EXTRA_API MaaJSCustomControllerCreate(MaaJSCustomControllerHandle handle,
                                                                  MaaControllerCallback callback)
    {
        return MaaCustomControllerCreate(&__custom_controller_api, callback, handle);
    }
}
