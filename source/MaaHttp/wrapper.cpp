// clang-format off

#include "Utils.h"
#define LHG_PROCESS

// LHG SEC BEGIN lhg.include
#include "MaaFramework/MaaAPI.h"
#include "MaaToolkit/MaaToolkitAPI.h"
// LHG SEC END

// LHG SEC BEGIN lhg.custom.global
namespace lhg {
    
    template <>
    inline MaaStringBuffer *output_prepare<MaaStringBuffer *>() {
        return MaaCreateStringBuffer();
    }

    template <>
    inline json::value output_finalize<MaaStringBuffer *>(MaaStringBuffer *v) {
        size_t len = MaaGetStringSize(v);
        std::string res(MaaGetString(v), len);
        MaaDestroyStringBuffer(v);
        return res;
    }

    template <>
    struct schema_t<MaaStringBuffer *>
    {
        static constexpr const char* const schema = "string";
    };

}
// LHG SEC END

static lhg::callback_manager<void (*)(const char *, const char *, void *)> MaaAPICallback__Manager;

static lhg::opaque_manager<MaaControllerAPI *> MaaControllerAPI__OpaqueManager;

template <>
struct lhg::schema_t<MaaControllerAPI *>
{
    static constexpr const char* const schema = "string@MaaControllerAPI";
};

static lhg::opaque_manager<MaaResourceAPI *> MaaResourceAPI__OpaqueManager;

template <>
struct lhg::schema_t<MaaResourceAPI *>
{
    static constexpr const char* const schema = "string@MaaResourceAPI";
};

static lhg::opaque_manager<MaaInstanceAPI *> MaaInstanceAPI__OpaqueManager;

template <>
struct lhg::schema_t<MaaInstanceAPI *>
{
    static constexpr const char* const schema = "string@MaaInstanceAPI";
};

static lhg::opaque_manager<MaaImageBuffer *> MaaImageBuffer__OpaqueManager;

template <>
struct lhg::schema_t<MaaImageBuffer *>
{
    static constexpr const char* const schema = "string@MaaImageBuffer";
};

json::object MaaAdbControllerCreate_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaAdbControllerCreate.input.adb_path
// LHG SEC DEF
        { "adb_path", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaAdbControllerCreate.input.address
// LHG SEC DEF
        { "address", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaAdbControllerCreate.input.type
// LHG SEC DEF
        { "type", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaAdbControllerCreate.input.config
// LHG SEC DEF
        { "config", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaAdbControllerCreate.input.callback
// LHG SEC DEF
        { "callback", "string@MaaAPICallback" },
// LHG SEC END
    };
}

json::object MaaWin32ControllerCreate_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaWin32ControllerCreate.input.hWnd
// LHG SEC DEF
        { "hWnd", lhg::schema_t<unsigned long long>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaWin32ControllerCreate.input.type
// LHG SEC DEF
        { "type", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaWin32ControllerCreate.input.callback
// LHG SEC DEF
        { "callback", "string@MaaAPICallback" },
// LHG SEC END
    };
}

json::object MaaAdbControllerCreateV2_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaAdbControllerCreateV2.input.adb_path
// LHG SEC DEF
        { "adb_path", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaAdbControllerCreateV2.input.address
// LHG SEC DEF
        { "address", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaAdbControllerCreateV2.input.type
// LHG SEC DEF
        { "type", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaAdbControllerCreateV2.input.config
// LHG SEC DEF
        { "config", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaAdbControllerCreateV2.input.agent_path
// LHG SEC DEF
        { "agent_path", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaAdbControllerCreateV2.input.callback
// LHG SEC DEF
        { "callback", "string@MaaAPICallback" },
// LHG SEC END
    };
}

json::object MaaThriftControllerCreate_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaThriftControllerCreate.input.type
// LHG SEC DEF
        { "type", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaThriftControllerCreate.input.host
// LHG SEC DEF
        { "host", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaThriftControllerCreate.input.port
// LHG SEC DEF
        { "port", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaThriftControllerCreate.input.config
// LHG SEC DEF
        { "config", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaThriftControllerCreate.input.callback
// LHG SEC DEF
        { "callback", "string@MaaAPICallback" },
// LHG SEC END
    };
}

json::object MaaDbgControllerCreate_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaDbgControllerCreate.input.read_path
// LHG SEC DEF
        { "read_path", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaDbgControllerCreate.input.write_path
// LHG SEC DEF
        { "write_path", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaDbgControllerCreate.input.type
// LHG SEC DEF
        { "type", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaDbgControllerCreate.input.config
// LHG SEC DEF
        { "config", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaDbgControllerCreate.input.callback
// LHG SEC DEF
        { "callback", "string@MaaAPICallback" },
// LHG SEC END
    };
}

json::object MaaControllerDestroy_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaControllerDestroy.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
    };
}

json::object MaaControllerSetOption_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaControllerSetOption.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerSetOption.input.key
// LHG SEC DEF
        { "key", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerSetOption.input.value
        { "value", "string|number|boolean" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerSetOption.input.val_size

// LHG SEC END
    };
}

json::object MaaControllerPostConnection_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaControllerPostConnection.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
    };
}

json::object MaaControllerPostClick_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaControllerPostClick.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostClick.input.x
// LHG SEC DEF
        { "x", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostClick.input.y
// LHG SEC DEF
        { "y", lhg::schema_t<int>::schema },
// LHG SEC END
    };
}

json::object MaaControllerPostSwipe_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaControllerPostSwipe.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostSwipe.input.x1
// LHG SEC DEF
        { "x1", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostSwipe.input.y1
// LHG SEC DEF
        { "y1", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostSwipe.input.x2
// LHG SEC DEF
        { "x2", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostSwipe.input.y2
// LHG SEC DEF
        { "y2", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostSwipe.input.duration
// LHG SEC DEF
        { "duration", lhg::schema_t<int>::schema },
// LHG SEC END
    };
}

json::object MaaControllerPostPressKey_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaControllerPostPressKey.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostPressKey.input.keycode
// LHG SEC DEF
        { "keycode", lhg::schema_t<int>::schema },
// LHG SEC END
    };
}

json::object MaaControllerPostInputText_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaControllerPostInputText.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostInputText.input.text
// LHG SEC DEF
        { "text", lhg::schema_t<const char *>::schema },
// LHG SEC END
    };
}

json::object MaaControllerPostTouchDown_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaControllerPostTouchDown.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostTouchDown.input.contact
// LHG SEC DEF
        { "contact", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostTouchDown.input.x
// LHG SEC DEF
        { "x", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostTouchDown.input.y
// LHG SEC DEF
        { "y", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostTouchDown.input.pressure
// LHG SEC DEF
        { "pressure", lhg::schema_t<int>::schema },
// LHG SEC END
    };
}

json::object MaaControllerPostTouchMove_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaControllerPostTouchMove.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostTouchMove.input.contact
// LHG SEC DEF
        { "contact", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostTouchMove.input.x
// LHG SEC DEF
        { "x", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostTouchMove.input.y
// LHG SEC DEF
        { "y", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostTouchMove.input.pressure
// LHG SEC DEF
        { "pressure", lhg::schema_t<int>::schema },
// LHG SEC END
    };
}

json::object MaaControllerPostTouchUp_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaControllerPostTouchUp.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerPostTouchUp.input.contact
// LHG SEC DEF
        { "contact", lhg::schema_t<int>::schema },
// LHG SEC END
    };
}

json::object MaaControllerPostScreencap_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaControllerPostScreencap.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
    };
}

json::object MaaControllerStatus_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaControllerStatus.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerStatus.input.id
// LHG SEC DEF
        { "id", lhg::schema_t<long long>::schema },
// LHG SEC END
    };
}

json::object MaaControllerWait_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaControllerWait.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerWait.input.id
// LHG SEC DEF
        { "id", lhg::schema_t<long long>::schema },
// LHG SEC END
    };
}

json::object MaaControllerConnected_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaControllerConnected.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
    };
}

json::object MaaControllerGetImage_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaControllerGetImage.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerGetImage.input.buffer
// LHG SEC DEF
        { "buffer", "string@MaaImageBuffer" },
// LHG SEC END
    };
}

json::object MaaControllerGetUUID_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaControllerGetUUID.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
    };
}

json::object MaaCreate_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaCreate.input.callback
// LHG SEC DEF
        { "callback", "string@MaaAPICallback" },
// LHG SEC END
    };
}

json::object MaaDestroy_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaDestroy.input.inst
// LHG SEC DEF
        { "inst", "string@MaaInstanceAPI" },
// LHG SEC END
    };
}

json::object MaaSetOption_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaSetOption.input.inst
// LHG SEC DEF
        { "inst", "string@MaaInstanceAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaSetOption.input.key
// LHG SEC DEF
        { "key", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaSetOption.input.value
        { "value", "string|number|boolean" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaSetOption.input.val_size

// LHG SEC END
    };
}

json::object MaaBindResource_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaBindResource.input.inst
// LHG SEC DEF
        { "inst", "string@MaaInstanceAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaBindResource.input.res
// LHG SEC DEF
        { "res", "string@MaaResourceAPI" },
// LHG SEC END
    };
}

json::object MaaBindController_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaBindController.input.inst
// LHG SEC DEF
        { "inst", "string@MaaInstanceAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaBindController.input.ctrl
// LHG SEC DEF
        { "ctrl", "string@MaaControllerAPI" },
// LHG SEC END
    };
}

json::object MaaInited_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaInited.input.inst
// LHG SEC DEF
        { "inst", "string@MaaInstanceAPI" },
// LHG SEC END
    };
}

json::object MaaClearCustomRecognizer_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaClearCustomRecognizer.input.inst
// LHG SEC DEF
        { "inst", "string@MaaInstanceAPI" },
// LHG SEC END
    };
}

json::object MaaClearCustomAction_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaClearCustomAction.input.inst
// LHG SEC DEF
        { "inst", "string@MaaInstanceAPI" },
// LHG SEC END
    };
}

json::object MaaPostTask_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaPostTask.input.inst
// LHG SEC DEF
        { "inst", "string@MaaInstanceAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaPostTask.input.entry
// LHG SEC DEF
        { "entry", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaPostTask.input.param
// LHG SEC DEF
        { "param", lhg::schema_t<const char *>::schema },
// LHG SEC END
    };
}

json::object MaaSetTaskParam_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaSetTaskParam.input.inst
// LHG SEC DEF
        { "inst", "string@MaaInstanceAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaSetTaskParam.input.id
// LHG SEC DEF
        { "id", lhg::schema_t<long long>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaSetTaskParam.input.param
// LHG SEC DEF
        { "param", lhg::schema_t<const char *>::schema },
// LHG SEC END
    };
}

json::object MaaTaskStatus_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaTaskStatus.input.inst
// LHG SEC DEF
        { "inst", "string@MaaInstanceAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaTaskStatus.input.id
// LHG SEC DEF
        { "id", lhg::schema_t<long long>::schema },
// LHG SEC END
    };
}

json::object MaaWaitTask_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaWaitTask.input.inst
// LHG SEC DEF
        { "inst", "string@MaaInstanceAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaWaitTask.input.id
// LHG SEC DEF
        { "id", lhg::schema_t<long long>::schema },
// LHG SEC END
    };
}

json::object MaaTaskAllFinished_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaTaskAllFinished.input.inst
// LHG SEC DEF
        { "inst", "string@MaaInstanceAPI" },
// LHG SEC END
    };
}

json::object MaaPostStop_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaPostStop.input.inst
// LHG SEC DEF
        { "inst", "string@MaaInstanceAPI" },
// LHG SEC END
    };
}

json::object MaaStop_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaStop.input.inst
// LHG SEC DEF
        { "inst", "string@MaaInstanceAPI" },
// LHG SEC END
    };
}

json::object MaaGetResource_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaGetResource.input.inst
// LHG SEC DEF
        { "inst", "string@MaaInstanceAPI" },
// LHG SEC END
    };
}

json::object MaaGetController_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaGetController.input.inst
// LHG SEC DEF
        { "inst", "string@MaaInstanceAPI" },
// LHG SEC END
    };
}

json::object MaaResourceCreate_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaResourceCreate.input.callback
// LHG SEC DEF
        { "callback", "string@MaaAPICallback" },
// LHG SEC END
    };
}

json::object MaaResourceDestroy_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaResourceDestroy.input.res
// LHG SEC DEF
        { "res", "string@MaaResourceAPI" },
// LHG SEC END
    };
}

json::object MaaResourcePostPath_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaResourcePostPath.input.res
// LHG SEC DEF
        { "res", "string@MaaResourceAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaResourcePostPath.input.path
// LHG SEC DEF
        { "path", lhg::schema_t<const char *>::schema },
// LHG SEC END
    };
}

json::object MaaResourceStatus_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaResourceStatus.input.res
// LHG SEC DEF
        { "res", "string@MaaResourceAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaResourceStatus.input.id
// LHG SEC DEF
        { "id", lhg::schema_t<long long>::schema },
// LHG SEC END
    };
}

json::object MaaResourceWait_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaResourceWait.input.res
// LHG SEC DEF
        { "res", "string@MaaResourceAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaResourceWait.input.id
// LHG SEC DEF
        { "id", lhg::schema_t<long long>::schema },
// LHG SEC END
    };
}

json::object MaaResourceLoaded_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaResourceLoaded.input.res
// LHG SEC DEF
        { "res", "string@MaaResourceAPI" },
// LHG SEC END
    };
}

json::object MaaResourceSetOption_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaResourceSetOption.input.res
// LHG SEC DEF
        { "res", "string@MaaResourceAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaResourceSetOption.input.key
// LHG SEC DEF
        { "key", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaResourceSetOption.input.value
        { "value", "string|number|boolean" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaResourceSetOption.input.val_size

// LHG SEC END
    };
}

json::object MaaResourceGetHash_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaResourceGetHash.input.res
// LHG SEC DEF
        { "res", "string@MaaResourceAPI" },
// LHG SEC END
    };
}

json::object MaaResourceGetTaskList_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaResourceGetTaskList.input.res
// LHG SEC DEF
        { "res", "string@MaaResourceAPI" },
// LHG SEC END
    };
}

json::object MaaCreateImageBuffer_HelperInput() {
    return json::object {
    };
}

json::object MaaDestroyImageBuffer_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaDestroyImageBuffer.input.handle
// LHG SEC DEF
        { "handle", "string@MaaImageBuffer" },
// LHG SEC END
    };
}

json::object MaaIsImageEmpty_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaIsImageEmpty.input.handle
// LHG SEC DEF
        { "handle", "string@MaaImageBuffer" },
// LHG SEC END
    };
}

json::object MaaClearImage_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaClearImage.input.handle
// LHG SEC DEF
        { "handle", "string@MaaImageBuffer" },
// LHG SEC END
    };
}

json::object MaaGetImageWidth_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaGetImageWidth.input.handle
// LHG SEC DEF
        { "handle", "string@MaaImageBuffer" },
// LHG SEC END
    };
}

json::object MaaGetImageHeight_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaGetImageHeight.input.handle
// LHG SEC DEF
        { "handle", "string@MaaImageBuffer" },
// LHG SEC END
    };
}

json::object MaaGetImageType_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaGetImageType.input.handle
// LHG SEC DEF
        { "handle", "string@MaaImageBuffer" },
// LHG SEC END
    };
}

json::object MaaGetImageEncoded_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaGetImageEncoded.input.handle
// LHG SEC DEF
        { "handle", "string@MaaImageBuffer" },
// LHG SEC END
    };
}

json::object MaaSetImageEncoded_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaSetImageEncoded.input.handle
// LHG SEC DEF
        { "handle", "string@MaaImageBuffer" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaSetImageEncoded.input.data
        { "data", "string@buffer" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaSetImageEncoded.input.size

// LHG SEC END
    };
}

json::object MaaVersion_HelperInput() {
    return json::object {
    };
}

json::object MaaSetGlobalOption_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaSetGlobalOption.input.key
// LHG SEC DEF
        { "key", lhg::schema_t<int>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaSetGlobalOption.input.value
        { "value", "string|number|boolean" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaSetGlobalOption.input.val_size

// LHG SEC END
    };
}

json::object MaaToolkitInit_HelperInput() {
    return json::object {
    };
}

json::object MaaToolkitUninit_HelperInput() {
    return json::object {
    };
}

json::object MaaToolkitFindDevice_HelperInput() {
    return json::object {
    };
}

json::object MaaToolkitFindDeviceWithAdb_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaToolkitFindDeviceWithAdb.input.adb_path
// LHG SEC DEF
        { "adb_path", lhg::schema_t<const char *>::schema },
// LHG SEC END
    };
}

json::object MaaToolkitPostFindDevice_HelperInput() {
    return json::object {
    };
}

json::object MaaToolkitPostFindDeviceWithAdb_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaToolkitPostFindDeviceWithAdb.input.adb_path
// LHG SEC DEF
        { "adb_path", lhg::schema_t<const char *>::schema },
// LHG SEC END
    };
}

json::object MaaToolkitIsFindDeviceCompleted_HelperInput() {
    return json::object {
    };
}

json::object MaaToolkitWaitForFindDeviceToComplete_HelperInput() {
    return json::object {
    };
}

json::object MaaToolkitGetDeviceCount_HelperInput() {
    return json::object {
    };
}

json::object MaaToolkitGetDeviceName_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaToolkitGetDeviceName.input.index
// LHG SEC DEF
        { "index", lhg::schema_t<unsigned long long>::schema },
// LHG SEC END
    };
}

json::object MaaToolkitGetDeviceAdbPath_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaToolkitGetDeviceAdbPath.input.index
// LHG SEC DEF
        { "index", lhg::schema_t<unsigned long long>::schema },
// LHG SEC END
    };
}

json::object MaaToolkitGetDeviceAdbSerial_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaToolkitGetDeviceAdbSerial.input.index
// LHG SEC DEF
        { "index", lhg::schema_t<unsigned long long>::schema },
// LHG SEC END
    };
}

json::object MaaToolkitGetDeviceAdbControllerType_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaToolkitGetDeviceAdbControllerType.input.index
// LHG SEC DEF
        { "index", lhg::schema_t<unsigned long long>::schema },
// LHG SEC END
    };
}

json::object MaaToolkitGetDeviceAdbConfig_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaToolkitGetDeviceAdbConfig.input.index
// LHG SEC DEF
        { "index", lhg::schema_t<unsigned long long>::schema },
// LHG SEC END
    };
}

json::object MaaToolkitRegisterCustomRecognizerExecutor_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaToolkitRegisterCustomRecognizerExecutor.input.handle
// LHG SEC DEF
        { "handle", "string@MaaInstanceAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaToolkitRegisterCustomRecognizerExecutor.input.recognizer_name
// LHG SEC DEF
        { "recognizer_name", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaToolkitRegisterCustomRecognizerExecutor.input.recognizer_exec_path
// LHG SEC DEF
        { "recognizer_exec_path", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaToolkitRegisterCustomRecognizerExecutor.input.recognizer_exec_param_json
// LHG SEC DEF
        { "recognizer_exec_param_json", lhg::schema_t<const char *>::schema },
// LHG SEC END
    };
}

json::object MaaToolkitUnregisterCustomRecognizerExecutor_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaToolkitUnregisterCustomRecognizerExecutor.input.handle
// LHG SEC DEF
        { "handle", "string@MaaInstanceAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaToolkitUnregisterCustomRecognizerExecutor.input.recognizer_name
// LHG SEC DEF
        { "recognizer_name", lhg::schema_t<const char *>::schema },
// LHG SEC END
    };
}

json::object MaaToolkitRegisterCustomActionExecutor_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaToolkitRegisterCustomActionExecutor.input.handle
// LHG SEC DEF
        { "handle", "string@MaaInstanceAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaToolkitRegisterCustomActionExecutor.input.action_name
// LHG SEC DEF
        { "action_name", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaToolkitRegisterCustomActionExecutor.input.action_exec_path
// LHG SEC DEF
        { "action_exec_path", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaToolkitRegisterCustomActionExecutor.input.action_exec_param_json
// LHG SEC DEF
        { "action_exec_param_json", lhg::schema_t<const char *>::schema },
// LHG SEC END
    };
}

json::object MaaToolkitUnregisterCustomActionExecutor_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaToolkitUnregisterCustomActionExecutor.input.handle
// LHG SEC DEF
        { "handle", "string@MaaInstanceAPI" },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaToolkitUnregisterCustomActionExecutor.input.action_name
// LHG SEC DEF
        { "action_name", lhg::schema_t<const char *>::schema },
// LHG SEC END
    };
}

json::object MaaToolkitFindWindow_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaToolkitFindWindow.input.class_name
// LHG SEC DEF
        { "class_name", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaToolkitFindWindow.input.window_name
// LHG SEC DEF
        { "window_name", lhg::schema_t<const char *>::schema },
// LHG SEC END
    };
}

json::object MaaToolkitSearchWindow_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaToolkitSearchWindow.input.class_name
// LHG SEC DEF
        { "class_name", lhg::schema_t<const char *>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaToolkitSearchWindow.input.window_name
// LHG SEC DEF
        { "window_name", lhg::schema_t<const char *>::schema },
// LHG SEC END
    };
}

json::object MaaToolkitGetWindow_HelperInput() {
    return json::object {
// LHG SEC BEGIN lhg.helper.MaaToolkitGetWindow.input.index
// LHG SEC DEF
        { "index", lhg::schema_t<unsigned long long>::schema },
// LHG SEC END
    };
}

json::object MaaToolkitGetCursorWindow_HelperInput() {
    return json::object {
    };
}

json::object MaaAdbControllerCreate_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaAdbControllerCreate.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<MaaControllerAPI *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaWin32ControllerCreate_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaWin32ControllerCreate.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<MaaControllerAPI *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaAdbControllerCreateV2_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaAdbControllerCreateV2.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<MaaControllerAPI *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaThriftControllerCreate_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaThriftControllerCreate.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<MaaControllerAPI *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaDbgControllerCreate_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaDbgControllerCreate.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<MaaControllerAPI *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaControllerDestroy_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaControllerDestroy.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<void>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaControllerSetOption_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaControllerSetOption.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaControllerPostConnection_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaControllerPostConnection.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaControllerPostClick_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaControllerPostClick.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaControllerPostSwipe_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaControllerPostSwipe.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaControllerPostPressKey_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaControllerPostPressKey.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaControllerPostInputText_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaControllerPostInputText.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaControllerPostTouchDown_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaControllerPostTouchDown.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaControllerPostTouchMove_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaControllerPostTouchMove.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaControllerPostTouchUp_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaControllerPostTouchUp.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaControllerPostScreencap_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaControllerPostScreencap.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaControllerStatus_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaControllerStatus.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<int>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaControllerWait_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaControllerWait.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<int>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaControllerConnected_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaControllerConnected.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaControllerGetImage_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaControllerGetImage.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaControllerGetUUID_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaControllerGetUUID.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaControllerGetUUID.output.buffer
// LHG SEC DEF
            { "buffer", lhg::schema_t<MaaStringBuffer *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaCreate_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaCreate.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<MaaInstanceAPI *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaDestroy_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaDestroy.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<void>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaSetOption_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaSetOption.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaBindResource_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaBindResource.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaBindController_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaBindController.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaInited_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaInited.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaClearCustomRecognizer_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaClearCustomRecognizer.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaClearCustomAction_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaClearCustomAction.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaPostTask_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaPostTask.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaSetTaskParam_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaSetTaskParam.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaTaskStatus_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaTaskStatus.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<int>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaWaitTask_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaWaitTask.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<int>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaTaskAllFinished_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaTaskAllFinished.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaPostStop_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaPostStop.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaStop_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaStop.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaGetResource_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaGetResource.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<MaaResourceAPI *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaGetController_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaGetController.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<MaaControllerAPI *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaResourceCreate_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaResourceCreate.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<MaaResourceAPI *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaResourceDestroy_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaResourceDestroy.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<void>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaResourcePostPath_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaResourcePostPath.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaResourceStatus_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaResourceStatus.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<int>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaResourceWait_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaResourceWait.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<int>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaResourceLoaded_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaResourceLoaded.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaResourceSetOption_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaResourceSetOption.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaResourceGetHash_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaResourceGetHash.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaResourceGetHash.output.buffer
// LHG SEC DEF
            { "buffer", lhg::schema_t<MaaStringBuffer *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaResourceGetTaskList_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaResourceGetTaskList.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
// LHG SEC BEGIN lhg.helper.MaaResourceGetTaskList.output.buffer
// LHG SEC DEF
            { "buffer", lhg::schema_t<MaaStringBuffer *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaCreateImageBuffer_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaCreateImageBuffer.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<MaaImageBuffer *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaDestroyImageBuffer_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaDestroyImageBuffer.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<void>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaIsImageEmpty_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaIsImageEmpty.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaClearImage_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaClearImage.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaGetImageWidth_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaGetImageWidth.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<int>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaGetImageHeight_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaGetImageHeight.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<int>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaGetImageType_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaGetImageType.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<int>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaGetImageEncoded_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaGetImageEncoded.output.return
            { "return", "string@buffer" },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaSetImageEncoded_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaSetImageEncoded.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaVersion_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaVersion.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<const char *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaSetGlobalOption_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaSetGlobalOption.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitInit_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitInit.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitUninit_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitUninit.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitFindDevice_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitFindDevice.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitFindDeviceWithAdb_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitFindDeviceWithAdb.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitPostFindDevice_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitPostFindDevice.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitPostFindDeviceWithAdb_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitPostFindDeviceWithAdb.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitIsFindDeviceCompleted_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitIsFindDeviceCompleted.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitWaitForFindDeviceToComplete_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitWaitForFindDeviceToComplete.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitGetDeviceCount_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitGetDeviceCount.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitGetDeviceName_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitGetDeviceName.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<const char *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitGetDeviceAdbPath_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitGetDeviceAdbPath.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<const char *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitGetDeviceAdbSerial_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitGetDeviceAdbSerial.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<const char *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitGetDeviceAdbControllerType_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitGetDeviceAdbControllerType.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<int>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitGetDeviceAdbConfig_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitGetDeviceAdbConfig.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<const char *>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitRegisterCustomRecognizerExecutor_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitRegisterCustomRecognizerExecutor.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitUnregisterCustomRecognizerExecutor_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitUnregisterCustomRecognizerExecutor.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitRegisterCustomActionExecutor_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitRegisterCustomActionExecutor.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitUnregisterCustomActionExecutor_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitUnregisterCustomActionExecutor.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned char>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitFindWindow_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitFindWindow.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitSearchWindow_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitSearchWindow.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitGetWindow_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitGetWindow.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

json::object MaaToolkitGetCursorWindow_HelperOutput() {
    return json::object {
        { "data", {
// LHG SEC BEGIN lhg.helper.MaaToolkitGetCursorWindow.output.return
// LHG SEC DEF
            { "return", lhg::schema_t<unsigned long long>::schema },
// LHG SEC END
        }},
        { "error", "string" }
    };
}

std::optional<json::object> MaaAdbControllerCreate_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreate.arg.adb_path.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["adb_path"])) {
            __error = "adb_path should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreate.arg.address.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["address"])) {
            __error = "address should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreate.arg.type.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["type"])) {
            __error = "type should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreate.arg.config.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["config"])) {
            __error = "config should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreate.arg.callback_arg.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["null"])) {
        __error = "null should be string@MaaAPICallback";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreate.arg.adb_path
// LHG SEC DEF
    auto adb_path_temp = lhg::from_json<const char *>(__param["adb_path"]);
    auto adb_path = lhg::from_json_fix<const char *>(adb_path_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreate.arg.address
// LHG SEC DEF
    auto address_temp = lhg::from_json<const char *>(__param["address"]);
    auto address = lhg::from_json_fix<const char *>(address_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreate.arg.type
// LHG SEC DEF
    auto type_temp = lhg::from_json<int>(__param["type"]);
    auto type = lhg::from_json_fix<int>(type_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreate.arg.config
// LHG SEC DEF
    auto config_temp = lhg::from_json<const char *>(__param["config"]);
    auto config = lhg::from_json_fix<const char *>(config_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreate.arg.callback
// LHG SEC DEF
    auto callback = &lhg::callback_implementation<2, void (*)(const char *, const char *, void *), const char *, const char *, void *>;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreate.arg.callback_arg
// LHG SEC DEF
    auto callback_arg_id = __param["callback"].as_string();
    auto callback_arg = MaaAPICallback__Manager.find(callback_arg_id).get();
    if (callback_arg) {
        __error = "callback not found in manager";
        return std::nullopt;
    }
// LHG SEC END
    auto __return = MaaAdbControllerCreate(
        adb_path,
        address,
        type,
        config,
        callback,
        callback_arg
    );
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreate.return
// LHG SEC DEF
    auto __ret = MaaControllerAPI__OpaqueManager.add(__return);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreate.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaWin32ControllerCreate_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaWin32ControllerCreate.arg.hWnd.check
// LHG SEC DEF
    if constexpr (lhg::check_t<unsigned long long>::enable) {
        if (!lhg::check_var<unsigned long long>(__param["hWnd"])) {
            __error = "hWnd should be unsigned long long";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaWin32ControllerCreate.arg.type.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["type"])) {
            __error = "type should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaWin32ControllerCreate.arg.callback_arg.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["null"])) {
        __error = "null should be string@MaaAPICallback";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaWin32ControllerCreate.arg.hWnd
// LHG SEC DEF
    auto hWnd_temp = lhg::from_json<unsigned long long>(__param["hWnd"]);
    auto hWnd = lhg::from_json_fix<unsigned long long>(hWnd_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaWin32ControllerCreate.arg.type
// LHG SEC DEF
    auto type_temp = lhg::from_json<int>(__param["type"]);
    auto type = lhg::from_json_fix<int>(type_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaWin32ControllerCreate.arg.callback
// LHG SEC DEF
    auto callback = &lhg::callback_implementation<2, void (*)(const char *, const char *, void *), const char *, const char *, void *>;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaWin32ControllerCreate.arg.callback_arg
// LHG SEC DEF
    auto callback_arg_id = __param["callback"].as_string();
    auto callback_arg = MaaAPICallback__Manager.find(callback_arg_id).get();
    if (callback_arg) {
        __error = "callback not found in manager";
        return std::nullopt;
    }
// LHG SEC END
    auto __return = MaaWin32ControllerCreate(
        hWnd,
        type,
        callback,
        callback_arg
    );
// LHG SEC BEGIN lhg.impl.MaaWin32ControllerCreate.return
// LHG SEC DEF
    auto __ret = MaaControllerAPI__OpaqueManager.add(__return);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaWin32ControllerCreate.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaAdbControllerCreateV2_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.arg.adb_path.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["adb_path"])) {
            __error = "adb_path should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.arg.address.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["address"])) {
            __error = "address should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.arg.type.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["type"])) {
            __error = "type should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.arg.config.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["config"])) {
            __error = "config should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.arg.agent_path.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["agent_path"])) {
            __error = "agent_path should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.arg.callback_arg.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["null"])) {
        __error = "null should be string@MaaAPICallback";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.arg.adb_path
// LHG SEC DEF
    auto adb_path_temp = lhg::from_json<const char *>(__param["adb_path"]);
    auto adb_path = lhg::from_json_fix<const char *>(adb_path_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.arg.address
// LHG SEC DEF
    auto address_temp = lhg::from_json<const char *>(__param["address"]);
    auto address = lhg::from_json_fix<const char *>(address_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.arg.type
// LHG SEC DEF
    auto type_temp = lhg::from_json<int>(__param["type"]);
    auto type = lhg::from_json_fix<int>(type_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.arg.config
// LHG SEC DEF
    auto config_temp = lhg::from_json<const char *>(__param["config"]);
    auto config = lhg::from_json_fix<const char *>(config_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.arg.agent_path
// LHG SEC DEF
    auto agent_path_temp = lhg::from_json<const char *>(__param["agent_path"]);
    auto agent_path = lhg::from_json_fix<const char *>(agent_path_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.arg.callback
// LHG SEC DEF
    auto callback = &lhg::callback_implementation<2, void (*)(const char *, const char *, void *), const char *, const char *, void *>;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.arg.callback_arg
// LHG SEC DEF
    auto callback_arg_id = __param["callback"].as_string();
    auto callback_arg = MaaAPICallback__Manager.find(callback_arg_id).get();
    if (callback_arg) {
        __error = "callback not found in manager";
        return std::nullopt;
    }
// LHG SEC END
    auto __return = MaaAdbControllerCreateV2(
        adb_path,
        address,
        type,
        config,
        agent_path,
        callback,
        callback_arg
    );
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.return
// LHG SEC DEF
    auto __ret = MaaControllerAPI__OpaqueManager.add(__return);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaThriftControllerCreate_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaThriftControllerCreate.arg.type.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["type"])) {
            __error = "type should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaThriftControllerCreate.arg.host.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["host"])) {
            __error = "host should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaThriftControllerCreate.arg.port.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["port"])) {
            __error = "port should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaThriftControllerCreate.arg.config.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["config"])) {
            __error = "config should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaThriftControllerCreate.arg.callback_arg.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["null"])) {
        __error = "null should be string@MaaAPICallback";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaThriftControllerCreate.arg.type
// LHG SEC DEF
    auto type_temp = lhg::from_json<int>(__param["type"]);
    auto type = lhg::from_json_fix<int>(type_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaThriftControllerCreate.arg.host
// LHG SEC DEF
    auto host_temp = lhg::from_json<const char *>(__param["host"]);
    auto host = lhg::from_json_fix<const char *>(host_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaThriftControllerCreate.arg.port
// LHG SEC DEF
    auto port_temp = lhg::from_json<int>(__param["port"]);
    auto port = lhg::from_json_fix<int>(port_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaThriftControllerCreate.arg.config
// LHG SEC DEF
    auto config_temp = lhg::from_json<const char *>(__param["config"]);
    auto config = lhg::from_json_fix<const char *>(config_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaThriftControllerCreate.arg.callback
// LHG SEC DEF
    auto callback = &lhg::callback_implementation<2, void (*)(const char *, const char *, void *), const char *, const char *, void *>;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaThriftControllerCreate.arg.callback_arg
// LHG SEC DEF
    auto callback_arg_id = __param["callback"].as_string();
    auto callback_arg = MaaAPICallback__Manager.find(callback_arg_id).get();
    if (callback_arg) {
        __error = "callback not found in manager";
        return std::nullopt;
    }
// LHG SEC END
    auto __return = MaaThriftControllerCreate(
        type,
        host,
        port,
        config,
        callback,
        callback_arg
    );
// LHG SEC BEGIN lhg.impl.MaaThriftControllerCreate.return
// LHG SEC DEF
    auto __ret = MaaControllerAPI__OpaqueManager.add(__return);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaThriftControllerCreate.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaDbgControllerCreate_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaDbgControllerCreate.arg.read_path.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["read_path"])) {
            __error = "read_path should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaDbgControllerCreate.arg.write_path.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["write_path"])) {
            __error = "write_path should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaDbgControllerCreate.arg.type.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["type"])) {
            __error = "type should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaDbgControllerCreate.arg.config.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["config"])) {
            __error = "config should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaDbgControllerCreate.arg.callback_arg.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["null"])) {
        __error = "null should be string@MaaAPICallback";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaDbgControllerCreate.arg.read_path
// LHG SEC DEF
    auto read_path_temp = lhg::from_json<const char *>(__param["read_path"]);
    auto read_path = lhg::from_json_fix<const char *>(read_path_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaDbgControllerCreate.arg.write_path
// LHG SEC DEF
    auto write_path_temp = lhg::from_json<const char *>(__param["write_path"]);
    auto write_path = lhg::from_json_fix<const char *>(write_path_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaDbgControllerCreate.arg.type
// LHG SEC DEF
    auto type_temp = lhg::from_json<int>(__param["type"]);
    auto type = lhg::from_json_fix<int>(type_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaDbgControllerCreate.arg.config
// LHG SEC DEF
    auto config_temp = lhg::from_json<const char *>(__param["config"]);
    auto config = lhg::from_json_fix<const char *>(config_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaDbgControllerCreate.arg.callback
// LHG SEC DEF
    auto callback = &lhg::callback_implementation<2, void (*)(const char *, const char *, void *), const char *, const char *, void *>;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaDbgControllerCreate.arg.callback_arg
// LHG SEC DEF
    auto callback_arg_id = __param["callback"].as_string();
    auto callback_arg = MaaAPICallback__Manager.find(callback_arg_id).get();
    if (callback_arg) {
        __error = "callback not found in manager";
        return std::nullopt;
    }
// LHG SEC END
    auto __return = MaaDbgControllerCreate(
        read_path,
        write_path,
        type,
        config,
        callback,
        callback_arg
    );
// LHG SEC BEGIN lhg.impl.MaaDbgControllerCreate.return
// LHG SEC DEF
    auto __ret = MaaControllerAPI__OpaqueManager.add(__return);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaDbgControllerCreate.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaControllerDestroy_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaControllerDestroy.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerDestroy.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    MaaControllerAPI *ctrl = 0;
    MaaControllerAPI__OpaqueManager.del(ctrl_id, ctrl);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
    MaaControllerDestroy(
        ctrl
    );
// LHG SEC BEGIN lhg.impl.MaaControllerDestroy.return
// LHG SEC DEF

// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerDestroy.final
// LHG SEC DEF
    return json::object { { "return", json::value(json::value::value_type::null) },  };
// LHG SEC END
}

std::optional<json::object> MaaControllerSetOption_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaControllerSetOption.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerSetOption.arg.key.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["key"])) {
            __error = "key should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerSetOption.arg.value.check
    switch (__param["key"].as_integer()) {
        case MaaCtrlOption_DefaultAppPackageEntry:
        case MaaCtrlOption_DefaultAppPackage:
            if (!lhg::check_var<const char *>(__param["value"])) {
                __error = "value should be string";
                return std::nullopt;
            }
            break;
        case MaaCtrlOption_ScreenshotTargetLongSide:
        case MaaCtrlOption_ScreenshotTargetShortSide:
            if (!lhg::check_var<int>(__param["value"])) {
                __error = "value should be int";
                return std::nullopt;
            }
            break;
        case MaaCtrlOption_Recording:
            if (!lhg::check_var<bool>(__param["value"])) {
                __error = "value should be boolean";
                return std::nullopt;
            }
            break;
        default:
            __error = "key outside enum";
            return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerSetOption.arg.val_size.check

// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerSetOption.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    auto ctrl = MaaControllerAPI__OpaqueManager.get(ctrl_id);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaControllerSetOption.arg.key
// LHG SEC DEF
    auto key_temp = lhg::from_json<int>(__param["key"]);
    auto key = lhg::from_json_fix<int>(key_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerSetOption.arg.value

    std::string temp_str;
    bool temp_bool;
    int32_t temp_i32;
    void *value = 0;
    uint64_t val_size = 0;
    switch (key) {
        case MaaCtrlOption_DefaultAppPackageEntry:
        case MaaCtrlOption_DefaultAppPackage:
            temp_str = __param["value"].as_string();
            value = const_cast<char *>(temp_str.c_str());
            val_size = temp_str.size();
            break;
        case MaaCtrlOption_ScreenshotTargetLongSide:
        case MaaCtrlOption_ScreenshotTargetShortSide:
            temp_i32 = __param["value"].as_integer();
            value = &temp_i32;
            val_size = 4;
            break;
        case MaaCtrlOption_Recording:
            temp_bool = __param["value"].as_boolean();
            value = &temp_bool;
            val_size = 1;
            break;
        default:
            return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerSetOption.arg.val_size

// LHG SEC END
    auto __return = MaaControllerSetOption(
        ctrl,
        key,
        value,
        val_size
    );
// LHG SEC BEGIN lhg.impl.MaaControllerSetOption.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerSetOption.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaControllerPostConnection_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaControllerPostConnection.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostConnection.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    auto ctrl = MaaControllerAPI__OpaqueManager.get(ctrl_id);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaControllerPostConnection(
        ctrl
    );
// LHG SEC BEGIN lhg.impl.MaaControllerPostConnection.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostConnection.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaControllerPostClick_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaControllerPostClick.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostClick.arg.x.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["x"])) {
            __error = "x should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostClick.arg.y.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["y"])) {
            __error = "y should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostClick.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    auto ctrl = MaaControllerAPI__OpaqueManager.get(ctrl_id);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaControllerPostClick.arg.x
// LHG SEC DEF
    auto x_temp = lhg::from_json<int>(__param["x"]);
    auto x = lhg::from_json_fix<int>(x_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostClick.arg.y
// LHG SEC DEF
    auto y_temp = lhg::from_json<int>(__param["y"]);
    auto y = lhg::from_json_fix<int>(y_temp);
// LHG SEC END
    auto __return = MaaControllerPostClick(
        ctrl,
        x,
        y
    );
// LHG SEC BEGIN lhg.impl.MaaControllerPostClick.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostClick.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaControllerPostSwipe_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaControllerPostSwipe.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostSwipe.arg.x1.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["x1"])) {
            __error = "x1 should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostSwipe.arg.y1.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["y1"])) {
            __error = "y1 should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostSwipe.arg.x2.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["x2"])) {
            __error = "x2 should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostSwipe.arg.y2.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["y2"])) {
            __error = "y2 should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostSwipe.arg.duration.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["duration"])) {
            __error = "duration should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostSwipe.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    auto ctrl = MaaControllerAPI__OpaqueManager.get(ctrl_id);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaControllerPostSwipe.arg.x1
// LHG SEC DEF
    auto x1_temp = lhg::from_json<int>(__param["x1"]);
    auto x1 = lhg::from_json_fix<int>(x1_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostSwipe.arg.y1
// LHG SEC DEF
    auto y1_temp = lhg::from_json<int>(__param["y1"]);
    auto y1 = lhg::from_json_fix<int>(y1_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostSwipe.arg.x2
// LHG SEC DEF
    auto x2_temp = lhg::from_json<int>(__param["x2"]);
    auto x2 = lhg::from_json_fix<int>(x2_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostSwipe.arg.y2
// LHG SEC DEF
    auto y2_temp = lhg::from_json<int>(__param["y2"]);
    auto y2 = lhg::from_json_fix<int>(y2_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostSwipe.arg.duration
// LHG SEC DEF
    auto duration_temp = lhg::from_json<int>(__param["duration"]);
    auto duration = lhg::from_json_fix<int>(duration_temp);
// LHG SEC END
    auto __return = MaaControllerPostSwipe(
        ctrl,
        x1,
        y1,
        x2,
        y2,
        duration
    );
// LHG SEC BEGIN lhg.impl.MaaControllerPostSwipe.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostSwipe.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaControllerPostPressKey_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaControllerPostPressKey.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostPressKey.arg.keycode.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["keycode"])) {
            __error = "keycode should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostPressKey.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    auto ctrl = MaaControllerAPI__OpaqueManager.get(ctrl_id);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaControllerPostPressKey.arg.keycode
// LHG SEC DEF
    auto keycode_temp = lhg::from_json<int>(__param["keycode"]);
    auto keycode = lhg::from_json_fix<int>(keycode_temp);
// LHG SEC END
    auto __return = MaaControllerPostPressKey(
        ctrl,
        keycode
    );
// LHG SEC BEGIN lhg.impl.MaaControllerPostPressKey.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostPressKey.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaControllerPostInputText_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaControllerPostInputText.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostInputText.arg.text.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["text"])) {
            __error = "text should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostInputText.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    auto ctrl = MaaControllerAPI__OpaqueManager.get(ctrl_id);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaControllerPostInputText.arg.text
// LHG SEC DEF
    auto text_temp = lhg::from_json<const char *>(__param["text"]);
    auto text = lhg::from_json_fix<const char *>(text_temp);
// LHG SEC END
    auto __return = MaaControllerPostInputText(
        ctrl,
        text
    );
// LHG SEC BEGIN lhg.impl.MaaControllerPostInputText.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostInputText.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaControllerPostTouchDown_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchDown.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchDown.arg.contact.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["contact"])) {
            __error = "contact should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchDown.arg.x.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["x"])) {
            __error = "x should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchDown.arg.y.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["y"])) {
            __error = "y should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchDown.arg.pressure.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["pressure"])) {
            __error = "pressure should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchDown.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    auto ctrl = MaaControllerAPI__OpaqueManager.get(ctrl_id);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchDown.arg.contact
// LHG SEC DEF
    auto contact_temp = lhg::from_json<int>(__param["contact"]);
    auto contact = lhg::from_json_fix<int>(contact_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchDown.arg.x
// LHG SEC DEF
    auto x_temp = lhg::from_json<int>(__param["x"]);
    auto x = lhg::from_json_fix<int>(x_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchDown.arg.y
// LHG SEC DEF
    auto y_temp = lhg::from_json<int>(__param["y"]);
    auto y = lhg::from_json_fix<int>(y_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchDown.arg.pressure
// LHG SEC DEF
    auto pressure_temp = lhg::from_json<int>(__param["pressure"]);
    auto pressure = lhg::from_json_fix<int>(pressure_temp);
// LHG SEC END
    auto __return = MaaControllerPostTouchDown(
        ctrl,
        contact,
        x,
        y,
        pressure
    );
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchDown.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchDown.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaControllerPostTouchMove_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchMove.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchMove.arg.contact.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["contact"])) {
            __error = "contact should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchMove.arg.x.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["x"])) {
            __error = "x should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchMove.arg.y.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["y"])) {
            __error = "y should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchMove.arg.pressure.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["pressure"])) {
            __error = "pressure should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchMove.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    auto ctrl = MaaControllerAPI__OpaqueManager.get(ctrl_id);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchMove.arg.contact
// LHG SEC DEF
    auto contact_temp = lhg::from_json<int>(__param["contact"]);
    auto contact = lhg::from_json_fix<int>(contact_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchMove.arg.x
// LHG SEC DEF
    auto x_temp = lhg::from_json<int>(__param["x"]);
    auto x = lhg::from_json_fix<int>(x_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchMove.arg.y
// LHG SEC DEF
    auto y_temp = lhg::from_json<int>(__param["y"]);
    auto y = lhg::from_json_fix<int>(y_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchMove.arg.pressure
// LHG SEC DEF
    auto pressure_temp = lhg::from_json<int>(__param["pressure"]);
    auto pressure = lhg::from_json_fix<int>(pressure_temp);
// LHG SEC END
    auto __return = MaaControllerPostTouchMove(
        ctrl,
        contact,
        x,
        y,
        pressure
    );
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchMove.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchMove.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaControllerPostTouchUp_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchUp.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchUp.arg.contact.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["contact"])) {
            __error = "contact should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchUp.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    auto ctrl = MaaControllerAPI__OpaqueManager.get(ctrl_id);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchUp.arg.contact
// LHG SEC DEF
    auto contact_temp = lhg::from_json<int>(__param["contact"]);
    auto contact = lhg::from_json_fix<int>(contact_temp);
// LHG SEC END
    auto __return = MaaControllerPostTouchUp(
        ctrl,
        contact
    );
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchUp.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchUp.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaControllerPostScreencap_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaControllerPostScreencap.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostScreencap.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    auto ctrl = MaaControllerAPI__OpaqueManager.get(ctrl_id);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaControllerPostScreencap(
        ctrl
    );
// LHG SEC BEGIN lhg.impl.MaaControllerPostScreencap.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerPostScreencap.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaControllerStatus_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaControllerStatus.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerStatus.arg.id.check
// LHG SEC DEF
    if constexpr (lhg::check_t<long long>::enable) {
        if (!lhg::check_var<long long>(__param["id"])) {
            __error = "id should be long long";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerStatus.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    auto ctrl = MaaControllerAPI__OpaqueManager.get(ctrl_id);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaControllerStatus.arg.id
// LHG SEC DEF
    auto id_temp = lhg::from_json<long long>(__param["id"]);
    auto id = lhg::from_json_fix<long long>(id_temp);
// LHG SEC END
    auto __return = MaaControllerStatus(
        ctrl,
        id
    );
// LHG SEC BEGIN lhg.impl.MaaControllerStatus.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerStatus.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaControllerWait_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaControllerWait.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerWait.arg.id.check
// LHG SEC DEF
    if constexpr (lhg::check_t<long long>::enable) {
        if (!lhg::check_var<long long>(__param["id"])) {
            __error = "id should be long long";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerWait.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    auto ctrl = MaaControllerAPI__OpaqueManager.get(ctrl_id);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaControllerWait.arg.id
// LHG SEC DEF
    auto id_temp = lhg::from_json<long long>(__param["id"]);
    auto id = lhg::from_json_fix<long long>(id_temp);
// LHG SEC END
    auto __return = MaaControllerWait(
        ctrl,
        id
    );
// LHG SEC BEGIN lhg.impl.MaaControllerWait.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerWait.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaControllerConnected_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaControllerConnected.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerConnected.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    auto ctrl = MaaControllerAPI__OpaqueManager.get(ctrl_id);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaControllerConnected(
        ctrl
    );
// LHG SEC BEGIN lhg.impl.MaaControllerConnected.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerConnected.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaControllerGetImage_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaControllerGetImage.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerGetImage.arg.buffer.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["buffer"])) {
        __error = "buffer should be string@MaaImageBuffer";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerGetImage.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    auto ctrl = MaaControllerAPI__OpaqueManager.get(ctrl_id);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaControllerGetImage.arg.buffer
// LHG SEC DEF
    auto buffer_id = __param["buffer"].as_string();
    auto buffer = MaaImageBuffer__OpaqueManager.get(buffer_id);
// LHG SEC END
    if (!buffer) {
        __error = "buffer not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaControllerGetImage(
        ctrl,
        buffer
    );
// LHG SEC BEGIN lhg.impl.MaaControllerGetImage.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerGetImage.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaControllerGetUUID_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaControllerGetUUID.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerGetUUID.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    auto ctrl = MaaControllerAPI__OpaqueManager.get(ctrl_id);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaControllerGetUUID.arg.buffer
// LHG SEC DEF
    auto buffer = lhg::output_prepare<MaaStringBuffer *>();
// LHG SEC END
    auto __return = MaaControllerGetUUID(
        ctrl,
        buffer
    );
// LHG SEC BEGIN lhg.impl.MaaControllerGetUUID.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaControllerGetUUID.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) }, { "buffer", lhg::output_finalize(buffer) } };
// LHG SEC END
}

std::optional<json::object> MaaCreate_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaCreate.arg.callback_arg.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["null"])) {
        __error = "null should be string@MaaAPICallback";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaCreate.arg.callback
// LHG SEC DEF
    auto callback = &lhg::callback_implementation<2, void (*)(const char *, const char *, void *), const char *, const char *, void *>;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaCreate.arg.callback_arg
// LHG SEC DEF
    auto callback_arg_id = __param["callback"].as_string();
    auto callback_arg = MaaAPICallback__Manager.find(callback_arg_id).get();
    if (callback_arg) {
        __error = "callback not found in manager";
        return std::nullopt;
    }
// LHG SEC END
    auto __return = MaaCreate(
        callback,
        callback_arg
    );
// LHG SEC BEGIN lhg.impl.MaaCreate.return
// LHG SEC DEF
    auto __ret = MaaInstanceAPI__OpaqueManager.add(__return);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaCreate.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaDestroy_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaDestroy.arg.inst.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["inst"])) {
        __error = "inst should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaDestroy.arg.inst
// LHG SEC DEF
    auto inst_id = __param["inst"].as_string();
    MaaInstanceAPI *inst = 0;
    MaaInstanceAPI__OpaqueManager.del(inst_id, inst);
// LHG SEC END
    if (!inst) {
        __error = "inst not found in manager.";
        return std::nullopt;
    }
    MaaDestroy(
        inst
    );
// LHG SEC BEGIN lhg.impl.MaaDestroy.return
// LHG SEC DEF

// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaDestroy.final
// LHG SEC DEF
    return json::object { { "return", json::value(json::value::value_type::null) },  };
// LHG SEC END
}

std::optional<json::object> MaaSetOption_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaSetOption.arg.inst.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["inst"])) {
        __error = "inst should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetOption.arg.key.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["key"])) {
            __error = "key should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetOption.arg.value.check

// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetOption.arg.val_size.check

// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetOption.arg.inst
// LHG SEC DEF
    auto inst_id = __param["inst"].as_string();
    auto inst = MaaInstanceAPI__OpaqueManager.get(inst_id);
// LHG SEC END
    if (!inst) {
        __error = "inst not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaSetOption.arg.key
// LHG SEC DEF
    auto key_temp = lhg::from_json<int>(__param["key"]);
    auto key = lhg::from_json_fix<int>(key_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetOption.arg.value
    void *value;
    uint64_t val_size;
    __error = "no available key";
    return std::nullopt;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetOption.arg.val_size

// LHG SEC END
    auto __return = MaaSetOption(
        inst,
        key,
        value,
        val_size
    );
// LHG SEC BEGIN lhg.impl.MaaSetOption.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetOption.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaBindResource_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaBindResource.arg.inst.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["inst"])) {
        __error = "inst should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaBindResource.arg.res.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["res"])) {
        __error = "res should be string@MaaResourceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaBindResource.arg.inst
// LHG SEC DEF
    auto inst_id = __param["inst"].as_string();
    auto inst = MaaInstanceAPI__OpaqueManager.get(inst_id);
// LHG SEC END
    if (!inst) {
        __error = "inst not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaBindResource.arg.res
// LHG SEC DEF
    auto res_id = __param["res"].as_string();
    auto res = MaaResourceAPI__OpaqueManager.get(res_id);
// LHG SEC END
    if (!res) {
        __error = "res not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaBindResource(
        inst,
        res
    );
// LHG SEC BEGIN lhg.impl.MaaBindResource.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaBindResource.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaBindController_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaBindController.arg.inst.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["inst"])) {
        __error = "inst should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaBindController.arg.ctrl.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["ctrl"])) {
        __error = "ctrl should be string@MaaControllerAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaBindController.arg.inst
// LHG SEC DEF
    auto inst_id = __param["inst"].as_string();
    auto inst = MaaInstanceAPI__OpaqueManager.get(inst_id);
// LHG SEC END
    if (!inst) {
        __error = "inst not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaBindController.arg.ctrl
// LHG SEC DEF
    auto ctrl_id = __param["ctrl"].as_string();
    auto ctrl = MaaControllerAPI__OpaqueManager.get(ctrl_id);
// LHG SEC END
    if (!ctrl) {
        __error = "ctrl not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaBindController(
        inst,
        ctrl
    );
// LHG SEC BEGIN lhg.impl.MaaBindController.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaBindController.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaInited_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaInited.arg.inst.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["inst"])) {
        __error = "inst should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaInited.arg.inst
// LHG SEC DEF
    auto inst_id = __param["inst"].as_string();
    auto inst = MaaInstanceAPI__OpaqueManager.get(inst_id);
// LHG SEC END
    if (!inst) {
        __error = "inst not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaInited(
        inst
    );
// LHG SEC BEGIN lhg.impl.MaaInited.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaInited.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaClearCustomRecognizer_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaClearCustomRecognizer.arg.inst.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["inst"])) {
        __error = "inst should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaClearCustomRecognizer.arg.inst
// LHG SEC DEF
    auto inst_id = __param["inst"].as_string();
    auto inst = MaaInstanceAPI__OpaqueManager.get(inst_id);
// LHG SEC END
    if (!inst) {
        __error = "inst not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaClearCustomRecognizer(
        inst
    );
// LHG SEC BEGIN lhg.impl.MaaClearCustomRecognizer.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaClearCustomRecognizer.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaClearCustomAction_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaClearCustomAction.arg.inst.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["inst"])) {
        __error = "inst should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaClearCustomAction.arg.inst
// LHG SEC DEF
    auto inst_id = __param["inst"].as_string();
    auto inst = MaaInstanceAPI__OpaqueManager.get(inst_id);
// LHG SEC END
    if (!inst) {
        __error = "inst not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaClearCustomAction(
        inst
    );
// LHG SEC BEGIN lhg.impl.MaaClearCustomAction.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaClearCustomAction.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaPostTask_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaPostTask.arg.inst.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["inst"])) {
        __error = "inst should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaPostTask.arg.entry.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["entry"])) {
            __error = "entry should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaPostTask.arg.param.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["param"])) {
            __error = "param should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaPostTask.arg.inst
// LHG SEC DEF
    auto inst_id = __param["inst"].as_string();
    auto inst = MaaInstanceAPI__OpaqueManager.get(inst_id);
// LHG SEC END
    if (!inst) {
        __error = "inst not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaPostTask.arg.entry
// LHG SEC DEF
    auto entry_temp = lhg::from_json<const char *>(__param["entry"]);
    auto entry = lhg::from_json_fix<const char *>(entry_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaPostTask.arg.param
// LHG SEC DEF
    auto param_temp = lhg::from_json<const char *>(__param["param"]);
    auto param = lhg::from_json_fix<const char *>(param_temp);
// LHG SEC END
    auto __return = MaaPostTask(
        inst,
        entry,
        param
    );
// LHG SEC BEGIN lhg.impl.MaaPostTask.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaPostTask.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaSetTaskParam_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaSetTaskParam.arg.inst.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["inst"])) {
        __error = "inst should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetTaskParam.arg.id.check
// LHG SEC DEF
    if constexpr (lhg::check_t<long long>::enable) {
        if (!lhg::check_var<long long>(__param["id"])) {
            __error = "id should be long long";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetTaskParam.arg.param.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["param"])) {
            __error = "param should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetTaskParam.arg.inst
// LHG SEC DEF
    auto inst_id = __param["inst"].as_string();
    auto inst = MaaInstanceAPI__OpaqueManager.get(inst_id);
// LHG SEC END
    if (!inst) {
        __error = "inst not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaSetTaskParam.arg.id
// LHG SEC DEF
    auto id_temp = lhg::from_json<long long>(__param["id"]);
    auto id = lhg::from_json_fix<long long>(id_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetTaskParam.arg.param
// LHG SEC DEF
    auto param_temp = lhg::from_json<const char *>(__param["param"]);
    auto param = lhg::from_json_fix<const char *>(param_temp);
// LHG SEC END
    auto __return = MaaSetTaskParam(
        inst,
        id,
        param
    );
// LHG SEC BEGIN lhg.impl.MaaSetTaskParam.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetTaskParam.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaTaskStatus_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaTaskStatus.arg.inst.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["inst"])) {
        __error = "inst should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaTaskStatus.arg.id.check
// LHG SEC DEF
    if constexpr (lhg::check_t<long long>::enable) {
        if (!lhg::check_var<long long>(__param["id"])) {
            __error = "id should be long long";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaTaskStatus.arg.inst
// LHG SEC DEF
    auto inst_id = __param["inst"].as_string();
    auto inst = MaaInstanceAPI__OpaqueManager.get(inst_id);
// LHG SEC END
    if (!inst) {
        __error = "inst not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaTaskStatus.arg.id
// LHG SEC DEF
    auto id_temp = lhg::from_json<long long>(__param["id"]);
    auto id = lhg::from_json_fix<long long>(id_temp);
// LHG SEC END
    auto __return = MaaTaskStatus(
        inst,
        id
    );
// LHG SEC BEGIN lhg.impl.MaaTaskStatus.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaTaskStatus.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaWaitTask_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaWaitTask.arg.inst.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["inst"])) {
        __error = "inst should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaWaitTask.arg.id.check
// LHG SEC DEF
    if constexpr (lhg::check_t<long long>::enable) {
        if (!lhg::check_var<long long>(__param["id"])) {
            __error = "id should be long long";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaWaitTask.arg.inst
// LHG SEC DEF
    auto inst_id = __param["inst"].as_string();
    auto inst = MaaInstanceAPI__OpaqueManager.get(inst_id);
// LHG SEC END
    if (!inst) {
        __error = "inst not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaWaitTask.arg.id
// LHG SEC DEF
    auto id_temp = lhg::from_json<long long>(__param["id"]);
    auto id = lhg::from_json_fix<long long>(id_temp);
// LHG SEC END
    auto __return = MaaWaitTask(
        inst,
        id
    );
// LHG SEC BEGIN lhg.impl.MaaWaitTask.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaWaitTask.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaTaskAllFinished_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaTaskAllFinished.arg.inst.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["inst"])) {
        __error = "inst should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaTaskAllFinished.arg.inst
// LHG SEC DEF
    auto inst_id = __param["inst"].as_string();
    auto inst = MaaInstanceAPI__OpaqueManager.get(inst_id);
// LHG SEC END
    if (!inst) {
        __error = "inst not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaTaskAllFinished(
        inst
    );
// LHG SEC BEGIN lhg.impl.MaaTaskAllFinished.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaTaskAllFinished.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaPostStop_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaPostStop.arg.inst.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["inst"])) {
        __error = "inst should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaPostStop.arg.inst
// LHG SEC DEF
    auto inst_id = __param["inst"].as_string();
    auto inst = MaaInstanceAPI__OpaqueManager.get(inst_id);
// LHG SEC END
    if (!inst) {
        __error = "inst not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaPostStop(
        inst
    );
// LHG SEC BEGIN lhg.impl.MaaPostStop.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaPostStop.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaStop_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaStop.arg.inst.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["inst"])) {
        __error = "inst should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaStop.arg.inst
// LHG SEC DEF
    auto inst_id = __param["inst"].as_string();
    auto inst = MaaInstanceAPI__OpaqueManager.get(inst_id);
// LHG SEC END
    if (!inst) {
        __error = "inst not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaStop(
        inst
    );
// LHG SEC BEGIN lhg.impl.MaaStop.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaStop.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaGetResource_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaGetResource.arg.inst.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["inst"])) {
        __error = "inst should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaGetResource.arg.inst
// LHG SEC DEF
    auto inst_id = __param["inst"].as_string();
    auto inst = MaaInstanceAPI__OpaqueManager.get(inst_id);
// LHG SEC END
    if (!inst) {
        __error = "inst not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaGetResource(
        inst
    );
// LHG SEC BEGIN lhg.impl.MaaGetResource.return
// LHG SEC DEF
    auto __ret = MaaResourceAPI__OpaqueManager.add(__return);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaGetResource.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaGetController_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaGetController.arg.inst.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["inst"])) {
        __error = "inst should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaGetController.arg.inst
// LHG SEC DEF
    auto inst_id = __param["inst"].as_string();
    auto inst = MaaInstanceAPI__OpaqueManager.get(inst_id);
// LHG SEC END
    if (!inst) {
        __error = "inst not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaGetController(
        inst
    );
// LHG SEC BEGIN lhg.impl.MaaGetController.return
// LHG SEC DEF
    auto __ret = MaaControllerAPI__OpaqueManager.add(__return);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaGetController.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaResourceCreate_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaResourceCreate.arg.callback_arg.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["null"])) {
        __error = "null should be string@MaaAPICallback";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceCreate.arg.callback
// LHG SEC DEF
    auto callback = &lhg::callback_implementation<2, void (*)(const char *, const char *, void *), const char *, const char *, void *>;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceCreate.arg.callback_arg
// LHG SEC DEF
    auto callback_arg_id = __param["callback"].as_string();
    auto callback_arg = MaaAPICallback__Manager.find(callback_arg_id).get();
    if (callback_arg) {
        __error = "callback not found in manager";
        return std::nullopt;
    }
// LHG SEC END
    auto __return = MaaResourceCreate(
        callback,
        callback_arg
    );
// LHG SEC BEGIN lhg.impl.MaaResourceCreate.return
// LHG SEC DEF
    auto __ret = MaaResourceAPI__OpaqueManager.add(__return);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceCreate.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaResourceDestroy_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaResourceDestroy.arg.res.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["res"])) {
        __error = "res should be string@MaaResourceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceDestroy.arg.res
// LHG SEC DEF
    auto res_id = __param["res"].as_string();
    MaaResourceAPI *res = 0;
    MaaResourceAPI__OpaqueManager.del(res_id, res);
// LHG SEC END
    if (!res) {
        __error = "res not found in manager.";
        return std::nullopt;
    }
    MaaResourceDestroy(
        res
    );
// LHG SEC BEGIN lhg.impl.MaaResourceDestroy.return
// LHG SEC DEF

// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceDestroy.final
// LHG SEC DEF
    return json::object { { "return", json::value(json::value::value_type::null) },  };
// LHG SEC END
}

std::optional<json::object> MaaResourcePostPath_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaResourcePostPath.arg.res.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["res"])) {
        __error = "res should be string@MaaResourceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourcePostPath.arg.path.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["path"])) {
            __error = "path should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourcePostPath.arg.res
// LHG SEC DEF
    auto res_id = __param["res"].as_string();
    auto res = MaaResourceAPI__OpaqueManager.get(res_id);
// LHG SEC END
    if (!res) {
        __error = "res not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaResourcePostPath.arg.path
// LHG SEC DEF
    auto path_temp = lhg::from_json<const char *>(__param["path"]);
    auto path = lhg::from_json_fix<const char *>(path_temp);
// LHG SEC END
    auto __return = MaaResourcePostPath(
        res,
        path
    );
// LHG SEC BEGIN lhg.impl.MaaResourcePostPath.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourcePostPath.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaResourceStatus_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaResourceStatus.arg.res.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["res"])) {
        __error = "res should be string@MaaResourceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceStatus.arg.id.check
// LHG SEC DEF
    if constexpr (lhg::check_t<long long>::enable) {
        if (!lhg::check_var<long long>(__param["id"])) {
            __error = "id should be long long";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceStatus.arg.res
// LHG SEC DEF
    auto res_id = __param["res"].as_string();
    auto res = MaaResourceAPI__OpaqueManager.get(res_id);
// LHG SEC END
    if (!res) {
        __error = "res not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaResourceStatus.arg.id
// LHG SEC DEF
    auto id_temp = lhg::from_json<long long>(__param["id"]);
    auto id = lhg::from_json_fix<long long>(id_temp);
// LHG SEC END
    auto __return = MaaResourceStatus(
        res,
        id
    );
// LHG SEC BEGIN lhg.impl.MaaResourceStatus.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceStatus.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaResourceWait_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaResourceWait.arg.res.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["res"])) {
        __error = "res should be string@MaaResourceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceWait.arg.id.check
// LHG SEC DEF
    if constexpr (lhg::check_t<long long>::enable) {
        if (!lhg::check_var<long long>(__param["id"])) {
            __error = "id should be long long";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceWait.arg.res
// LHG SEC DEF
    auto res_id = __param["res"].as_string();
    auto res = MaaResourceAPI__OpaqueManager.get(res_id);
// LHG SEC END
    if (!res) {
        __error = "res not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaResourceWait.arg.id
// LHG SEC DEF
    auto id_temp = lhg::from_json<long long>(__param["id"]);
    auto id = lhg::from_json_fix<long long>(id_temp);
// LHG SEC END
    auto __return = MaaResourceWait(
        res,
        id
    );
// LHG SEC BEGIN lhg.impl.MaaResourceWait.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceWait.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaResourceLoaded_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaResourceLoaded.arg.res.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["res"])) {
        __error = "res should be string@MaaResourceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceLoaded.arg.res
// LHG SEC DEF
    auto res_id = __param["res"].as_string();
    auto res = MaaResourceAPI__OpaqueManager.get(res_id);
// LHG SEC END
    if (!res) {
        __error = "res not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaResourceLoaded(
        res
    );
// LHG SEC BEGIN lhg.impl.MaaResourceLoaded.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceLoaded.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaResourceSetOption_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaResourceSetOption.arg.res.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["res"])) {
        __error = "res should be string@MaaResourceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceSetOption.arg.key.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["key"])) {
            __error = "key should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceSetOption.arg.value.check

// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceSetOption.arg.val_size.check

// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceSetOption.arg.res
// LHG SEC DEF
    auto res_id = __param["res"].as_string();
    auto res = MaaResourceAPI__OpaqueManager.get(res_id);
// LHG SEC END
    if (!res) {
        __error = "res not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaResourceSetOption.arg.key
// LHG SEC DEF
    auto key_temp = lhg::from_json<int>(__param["key"]);
    auto key = lhg::from_json_fix<int>(key_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceSetOption.arg.value
    void *value;
    uint64_t val_size;
    __error = "no available key";
    return std::nullopt;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceSetOption.arg.val_size

// LHG SEC END
    auto __return = MaaResourceSetOption(
        res,
        key,
        value,
        val_size
    );
// LHG SEC BEGIN lhg.impl.MaaResourceSetOption.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceSetOption.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaResourceGetHash_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaResourceGetHash.arg.res.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["res"])) {
        __error = "res should be string@MaaResourceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceGetHash.arg.res
// LHG SEC DEF
    auto res_id = __param["res"].as_string();
    auto res = MaaResourceAPI__OpaqueManager.get(res_id);
// LHG SEC END
    if (!res) {
        __error = "res not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaResourceGetHash.arg.buffer
// LHG SEC DEF
    auto buffer = lhg::output_prepare<MaaStringBuffer *>();
// LHG SEC END
    auto __return = MaaResourceGetHash(
        res,
        buffer
    );
// LHG SEC BEGIN lhg.impl.MaaResourceGetHash.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceGetHash.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) }, { "buffer", lhg::output_finalize(buffer) } };
// LHG SEC END
}

std::optional<json::object> MaaResourceGetTaskList_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaResourceGetTaskList.arg.res.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["res"])) {
        __error = "res should be string@MaaResourceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceGetTaskList.arg.res
// LHG SEC DEF
    auto res_id = __param["res"].as_string();
    auto res = MaaResourceAPI__OpaqueManager.get(res_id);
// LHG SEC END
    if (!res) {
        __error = "res not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaResourceGetTaskList.arg.buffer
// LHG SEC DEF
    auto buffer = lhg::output_prepare<MaaStringBuffer *>();
// LHG SEC END
    auto __return = MaaResourceGetTaskList(
        res,
        buffer
    );
// LHG SEC BEGIN lhg.impl.MaaResourceGetTaskList.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaResourceGetTaskList.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) }, { "buffer", lhg::output_finalize(buffer) } };
// LHG SEC END
}

std::optional<json::object> MaaCreateImageBuffer_Wrapper(json::object __param, std::string &__error) {
    auto __return = MaaCreateImageBuffer(

    );
// LHG SEC BEGIN lhg.impl.MaaCreateImageBuffer.return
// LHG SEC DEF
    auto __ret = MaaImageBuffer__OpaqueManager.add(__return);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaCreateImageBuffer.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaDestroyImageBuffer_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaDestroyImageBuffer.arg.handle.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["handle"])) {
        __error = "handle should be string@MaaImageBuffer";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaDestroyImageBuffer.arg.handle
// LHG SEC DEF
    auto handle_id = __param["handle"].as_string();
    MaaImageBuffer *handle = 0;
    MaaImageBuffer__OpaqueManager.del(handle_id, handle);
// LHG SEC END
    if (!handle) {
        __error = "handle not found in manager.";
        return std::nullopt;
    }
    MaaDestroyImageBuffer(
        handle
    );
// LHG SEC BEGIN lhg.impl.MaaDestroyImageBuffer.return
// LHG SEC DEF

// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaDestroyImageBuffer.final
// LHG SEC DEF
    return json::object { { "return", json::value(json::value::value_type::null) },  };
// LHG SEC END
}

std::optional<json::object> MaaIsImageEmpty_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaIsImageEmpty.arg.handle.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["handle"])) {
        __error = "handle should be string@MaaImageBuffer";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaIsImageEmpty.arg.handle
// LHG SEC DEF
    auto handle_id = __param["handle"].as_string();
    auto handle = MaaImageBuffer__OpaqueManager.get(handle_id);
// LHG SEC END
    if (!handle) {
        __error = "handle not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaIsImageEmpty(
        handle
    );
// LHG SEC BEGIN lhg.impl.MaaIsImageEmpty.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaIsImageEmpty.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaClearImage_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaClearImage.arg.handle.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["handle"])) {
        __error = "handle should be string@MaaImageBuffer";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaClearImage.arg.handle
// LHG SEC DEF
    auto handle_id = __param["handle"].as_string();
    auto handle = MaaImageBuffer__OpaqueManager.get(handle_id);
// LHG SEC END
    if (!handle) {
        __error = "handle not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaClearImage(
        handle
    );
// LHG SEC BEGIN lhg.impl.MaaClearImage.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaClearImage.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaGetImageWidth_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaGetImageWidth.arg.handle.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["handle"])) {
        __error = "handle should be string@MaaImageBuffer";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaGetImageWidth.arg.handle
// LHG SEC DEF
    auto handle_id = __param["handle"].as_string();
    auto handle = MaaImageBuffer__OpaqueManager.get(handle_id);
// LHG SEC END
    if (!handle) {
        __error = "handle not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaGetImageWidth(
        handle
    );
// LHG SEC BEGIN lhg.impl.MaaGetImageWidth.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaGetImageWidth.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaGetImageHeight_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaGetImageHeight.arg.handle.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["handle"])) {
        __error = "handle should be string@MaaImageBuffer";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaGetImageHeight.arg.handle
// LHG SEC DEF
    auto handle_id = __param["handle"].as_string();
    auto handle = MaaImageBuffer__OpaqueManager.get(handle_id);
// LHG SEC END
    if (!handle) {
        __error = "handle not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaGetImageHeight(
        handle
    );
// LHG SEC BEGIN lhg.impl.MaaGetImageHeight.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaGetImageHeight.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaGetImageType_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaGetImageType.arg.handle.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["handle"])) {
        __error = "handle should be string@MaaImageBuffer";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaGetImageType.arg.handle
// LHG SEC DEF
    auto handle_id = __param["handle"].as_string();
    auto handle = MaaImageBuffer__OpaqueManager.get(handle_id);
// LHG SEC END
    if (!handle) {
        __error = "handle not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaGetImageType(
        handle
    );
// LHG SEC BEGIN lhg.impl.MaaGetImageType.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaGetImageType.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaGetImageEncoded_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaGetImageEncoded.arg.handle.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["handle"])) {
        __error = "handle should be string@MaaImageBuffer";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaGetImageEncoded.arg.handle
// LHG SEC DEF
    auto handle_id = __param["handle"].as_string();
    auto handle = MaaImageBuffer__OpaqueManager.get(handle_id);
// LHG SEC END
    if (!handle) {
        __error = "handle not found in manager.";
        return std::nullopt;
    }
    auto __return = MaaGetImageEncoded(
        handle
    );
// LHG SEC BEGIN lhg.impl.MaaGetImageEncoded.return
    std::string __ret(reinterpret_cast<char*>(__return), MaaGetImageEncodedSize(handle));
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaGetImageEncoded.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaSetImageEncoded_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaSetImageEncoded.arg.handle.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["handle"])) {
        __error = "handle should be string@MaaImageBuffer";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetImageEncoded.arg.data.check
    if (!lhg::check_var<const char *>(__param["data"])) {
        __error = "data should be string@buffer";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetImageEncoded.arg.size.check

// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetImageEncoded.arg.handle
// LHG SEC DEF
    auto handle_id = __param["handle"].as_string();
    auto handle = MaaImageBuffer__OpaqueManager.get(handle_id);
// LHG SEC END
    if (!handle) {
        __error = "handle not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaSetImageEncoded.arg.data
    auto data_temp = __param["data"].as_string();
    auto data = reinterpret_cast<uint8_t *>(const_cast<char *>(data_temp.c_str()));
    auto size = data_temp.size();
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetImageEncoded.arg.size

// LHG SEC END
    auto __return = MaaSetImageEncoded(
        handle,
        data,
        size
    );
// LHG SEC BEGIN lhg.impl.MaaSetImageEncoded.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetImageEncoded.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaVersion_Wrapper(json::object __param, std::string &__error) {
    auto __return = MaaVersion(

    );
// LHG SEC BEGIN lhg.impl.MaaVersion.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaVersion.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaSetGlobalOption_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaSetGlobalOption.arg.key.check
// LHG SEC DEF
    if constexpr (lhg::check_t<int>::enable) {
        if (!lhg::check_var<int>(__param["key"])) {
            __error = "key should be int";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetGlobalOption.arg.value.check
    switch (__param["key"].as_integer()) {
        case MaaGlobalOption_LogDir:
            if (!lhg::check_var<const char *>(__param["value"])) {
                __error = "value should be string";
                return std::nullopt;
            }
            break;
        case MaaGlobalOption_StdoutLevel:
            if (!lhg::check_var<int>(__param["value"])) {
                __error = "value should be int";
                return std::nullopt;
            }
            break;
        case MaaGlobalOption_SaveDraw:
        case MaaGlobalOption_Recording:
        case MaaGlobalOption_ShowHitDraw:
            if (!lhg::check_var<bool>(__param["value"])) {
                __error = "value should be boolean";
                return std::nullopt;
            }
            break;
        default:
            __error = "key outside enum";
            return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetGlobalOption.arg.val_size.check

// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetGlobalOption.arg.key
// LHG SEC DEF
    auto key_temp = lhg::from_json<int>(__param["key"]);
    auto key = lhg::from_json_fix<int>(key_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetGlobalOption.arg.value
    std::string temp_str;
    bool temp_bool;
    int32_t temp_i32;
    void *value = 0;
    uint64_t val_size = 0;
    switch (key) {
        case MaaGlobalOption_LogDir:
            temp_str = __param["value"].as_string();
            value = const_cast<char *>(temp_str.c_str());
            val_size = temp_str.size();
            break;
        case MaaGlobalOption_StdoutLevel:
            temp_i32 = __param["value"].as_integer();
            value = &temp_i32;
            val_size = 4;
            break;
        case MaaGlobalOption_SaveDraw:
        case MaaGlobalOption_Recording:
        case MaaGlobalOption_ShowHitDraw:
            temp_bool = __param["value"].as_boolean();
            value = &temp_bool;
            val_size = 1;
            break;
        default:
            return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetGlobalOption.arg.val_size

// LHG SEC END
    auto __return = MaaSetGlobalOption(
        key,
        value,
        val_size
    );
// LHG SEC BEGIN lhg.impl.MaaSetGlobalOption.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaSetGlobalOption.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitInit_Wrapper(json::object __param, std::string &__error) {
    auto __return = MaaToolkitInit(

    );
// LHG SEC BEGIN lhg.impl.MaaToolkitInit.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitInit.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitUninit_Wrapper(json::object __param, std::string &__error) {
    auto __return = MaaToolkitUninit(

    );
// LHG SEC BEGIN lhg.impl.MaaToolkitUninit.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitUninit.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitFindDevice_Wrapper(json::object __param, std::string &__error) {
    auto __return = MaaToolkitFindDevice(

    );
// LHG SEC BEGIN lhg.impl.MaaToolkitFindDevice.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitFindDevice.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitFindDeviceWithAdb_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaToolkitFindDeviceWithAdb.arg.adb_path.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["adb_path"])) {
            __error = "adb_path should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitFindDeviceWithAdb.arg.adb_path
// LHG SEC DEF
    auto adb_path_temp = lhg::from_json<const char *>(__param["adb_path"]);
    auto adb_path = lhg::from_json_fix<const char *>(adb_path_temp);
// LHG SEC END
    auto __return = MaaToolkitFindDeviceWithAdb(
        adb_path
    );
// LHG SEC BEGIN lhg.impl.MaaToolkitFindDeviceWithAdb.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitFindDeviceWithAdb.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitPostFindDevice_Wrapper(json::object __param, std::string &__error) {
    auto __return = MaaToolkitPostFindDevice(

    );
// LHG SEC BEGIN lhg.impl.MaaToolkitPostFindDevice.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitPostFindDevice.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitPostFindDeviceWithAdb_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaToolkitPostFindDeviceWithAdb.arg.adb_path.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["adb_path"])) {
            __error = "adb_path should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitPostFindDeviceWithAdb.arg.adb_path
// LHG SEC DEF
    auto adb_path_temp = lhg::from_json<const char *>(__param["adb_path"]);
    auto adb_path = lhg::from_json_fix<const char *>(adb_path_temp);
// LHG SEC END
    auto __return = MaaToolkitPostFindDeviceWithAdb(
        adb_path
    );
// LHG SEC BEGIN lhg.impl.MaaToolkitPostFindDeviceWithAdb.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitPostFindDeviceWithAdb.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitIsFindDeviceCompleted_Wrapper(json::object __param, std::string &__error) {
    auto __return = MaaToolkitIsFindDeviceCompleted(

    );
// LHG SEC BEGIN lhg.impl.MaaToolkitIsFindDeviceCompleted.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitIsFindDeviceCompleted.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitWaitForFindDeviceToComplete_Wrapper(json::object __param, std::string &__error) {
    auto __return = MaaToolkitWaitForFindDeviceToComplete(

    );
// LHG SEC BEGIN lhg.impl.MaaToolkitWaitForFindDeviceToComplete.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitWaitForFindDeviceToComplete.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitGetDeviceCount_Wrapper(json::object __param, std::string &__error) {
    auto __return = MaaToolkitGetDeviceCount(

    );
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceCount.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceCount.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitGetDeviceName_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceName.arg.index.check
// LHG SEC DEF
    if constexpr (lhg::check_t<unsigned long long>::enable) {
        if (!lhg::check_var<unsigned long long>(__param["index"])) {
            __error = "index should be unsigned long long";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceName.arg.index
// LHG SEC DEF
    auto index_temp = lhg::from_json<unsigned long long>(__param["index"]);
    auto index = lhg::from_json_fix<unsigned long long>(index_temp);
// LHG SEC END
    auto __return = MaaToolkitGetDeviceName(
        index
    );
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceName.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceName.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitGetDeviceAdbPath_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbPath.arg.index.check
// LHG SEC DEF
    if constexpr (lhg::check_t<unsigned long long>::enable) {
        if (!lhg::check_var<unsigned long long>(__param["index"])) {
            __error = "index should be unsigned long long";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbPath.arg.index
// LHG SEC DEF
    auto index_temp = lhg::from_json<unsigned long long>(__param["index"]);
    auto index = lhg::from_json_fix<unsigned long long>(index_temp);
// LHG SEC END
    auto __return = MaaToolkitGetDeviceAdbPath(
        index
    );
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbPath.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbPath.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitGetDeviceAdbSerial_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbSerial.arg.index.check
// LHG SEC DEF
    if constexpr (lhg::check_t<unsigned long long>::enable) {
        if (!lhg::check_var<unsigned long long>(__param["index"])) {
            __error = "index should be unsigned long long";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbSerial.arg.index
// LHG SEC DEF
    auto index_temp = lhg::from_json<unsigned long long>(__param["index"]);
    auto index = lhg::from_json_fix<unsigned long long>(index_temp);
// LHG SEC END
    auto __return = MaaToolkitGetDeviceAdbSerial(
        index
    );
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbSerial.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbSerial.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitGetDeviceAdbControllerType_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbControllerType.arg.index.check
// LHG SEC DEF
    if constexpr (lhg::check_t<unsigned long long>::enable) {
        if (!lhg::check_var<unsigned long long>(__param["index"])) {
            __error = "index should be unsigned long long";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbControllerType.arg.index
// LHG SEC DEF
    auto index_temp = lhg::from_json<unsigned long long>(__param["index"]);
    auto index = lhg::from_json_fix<unsigned long long>(index_temp);
// LHG SEC END
    auto __return = MaaToolkitGetDeviceAdbControllerType(
        index
    );
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbControllerType.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbControllerType.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitGetDeviceAdbConfig_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbConfig.arg.index.check
// LHG SEC DEF
    if constexpr (lhg::check_t<unsigned long long>::enable) {
        if (!lhg::check_var<unsigned long long>(__param["index"])) {
            __error = "index should be unsigned long long";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbConfig.arg.index
// LHG SEC DEF
    auto index_temp = lhg::from_json<unsigned long long>(__param["index"]);
    auto index = lhg::from_json_fix<unsigned long long>(index_temp);
// LHG SEC END
    auto __return = MaaToolkitGetDeviceAdbConfig(
        index
    );
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbConfig.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbConfig.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitRegisterCustomRecognizerExecutor_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomRecognizerExecutor.arg.handle.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["handle"])) {
        __error = "handle should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomRecognizerExecutor.arg.recognizer_name.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["recognizer_name"])) {
            __error = "recognizer_name should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomRecognizerExecutor.arg.recognizer_exec_path.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["recognizer_exec_path"])) {
            __error = "recognizer_exec_path should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomRecognizerExecutor.arg.recognizer_exec_param_json.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["recognizer_exec_param_json"])) {
            __error = "recognizer_exec_param_json should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomRecognizerExecutor.arg.handle
// LHG SEC DEF
    auto handle_id = __param["handle"].as_string();
    auto handle = MaaInstanceAPI__OpaqueManager.get(handle_id);
// LHG SEC END
    if (!handle) {
        __error = "handle not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomRecognizerExecutor.arg.recognizer_name
// LHG SEC DEF
    auto recognizer_name_temp = lhg::from_json<const char *>(__param["recognizer_name"]);
    auto recognizer_name = lhg::from_json_fix<const char *>(recognizer_name_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomRecognizerExecutor.arg.recognizer_exec_path
// LHG SEC DEF
    auto recognizer_exec_path_temp = lhg::from_json<const char *>(__param["recognizer_exec_path"]);
    auto recognizer_exec_path = lhg::from_json_fix<const char *>(recognizer_exec_path_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomRecognizerExecutor.arg.recognizer_exec_param_json
// LHG SEC DEF
    auto recognizer_exec_param_json_temp = lhg::from_json<const char *>(__param["recognizer_exec_param_json"]);
    auto recognizer_exec_param_json = lhg::from_json_fix<const char *>(recognizer_exec_param_json_temp);
// LHG SEC END
    auto __return = MaaToolkitRegisterCustomRecognizerExecutor(
        handle,
        recognizer_name,
        recognizer_exec_path,
        recognizer_exec_param_json
    );
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomRecognizerExecutor.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomRecognizerExecutor.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitUnregisterCustomRecognizerExecutor_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaToolkitUnregisterCustomRecognizerExecutor.arg.handle.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["handle"])) {
        __error = "handle should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitUnregisterCustomRecognizerExecutor.arg.recognizer_name.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["recognizer_name"])) {
            __error = "recognizer_name should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitUnregisterCustomRecognizerExecutor.arg.handle
// LHG SEC DEF
    auto handle_id = __param["handle"].as_string();
    auto handle = MaaInstanceAPI__OpaqueManager.get(handle_id);
// LHG SEC END
    if (!handle) {
        __error = "handle not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaToolkitUnregisterCustomRecognizerExecutor.arg.recognizer_name
// LHG SEC DEF
    auto recognizer_name_temp = lhg::from_json<const char *>(__param["recognizer_name"]);
    auto recognizer_name = lhg::from_json_fix<const char *>(recognizer_name_temp);
// LHG SEC END
    auto __return = MaaToolkitUnregisterCustomRecognizerExecutor(
        handle,
        recognizer_name
    );
// LHG SEC BEGIN lhg.impl.MaaToolkitUnregisterCustomRecognizerExecutor.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitUnregisterCustomRecognizerExecutor.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitRegisterCustomActionExecutor_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomActionExecutor.arg.handle.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["handle"])) {
        __error = "handle should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomActionExecutor.arg.action_name.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["action_name"])) {
            __error = "action_name should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomActionExecutor.arg.action_exec_path.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["action_exec_path"])) {
            __error = "action_exec_path should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomActionExecutor.arg.action_exec_param_json.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["action_exec_param_json"])) {
            __error = "action_exec_param_json should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomActionExecutor.arg.handle
// LHG SEC DEF
    auto handle_id = __param["handle"].as_string();
    auto handle = MaaInstanceAPI__OpaqueManager.get(handle_id);
// LHG SEC END
    if (!handle) {
        __error = "handle not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomActionExecutor.arg.action_name
// LHG SEC DEF
    auto action_name_temp = lhg::from_json<const char *>(__param["action_name"]);
    auto action_name = lhg::from_json_fix<const char *>(action_name_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomActionExecutor.arg.action_exec_path
// LHG SEC DEF
    auto action_exec_path_temp = lhg::from_json<const char *>(__param["action_exec_path"]);
    auto action_exec_path = lhg::from_json_fix<const char *>(action_exec_path_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomActionExecutor.arg.action_exec_param_json
// LHG SEC DEF
    auto action_exec_param_json_temp = lhg::from_json<const char *>(__param["action_exec_param_json"]);
    auto action_exec_param_json = lhg::from_json_fix<const char *>(action_exec_param_json_temp);
// LHG SEC END
    auto __return = MaaToolkitRegisterCustomActionExecutor(
        handle,
        action_name,
        action_exec_path,
        action_exec_param_json
    );
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomActionExecutor.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomActionExecutor.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitUnregisterCustomActionExecutor_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaToolkitUnregisterCustomActionExecutor.arg.handle.check
// LHG SEC DEF
    if (!lhg::check_var<const char*>(__param["handle"])) {
        __error = "handle should be string@MaaInstanceAPI";
        return std::nullopt;
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitUnregisterCustomActionExecutor.arg.action_name.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["action_name"])) {
            __error = "action_name should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitUnregisterCustomActionExecutor.arg.handle
// LHG SEC DEF
    auto handle_id = __param["handle"].as_string();
    auto handle = MaaInstanceAPI__OpaqueManager.get(handle_id);
// LHG SEC END
    if (!handle) {
        __error = "handle not found in manager.";
        return std::nullopt;
    }
// LHG SEC BEGIN lhg.impl.MaaToolkitUnregisterCustomActionExecutor.arg.action_name
// LHG SEC DEF
    auto action_name_temp = lhg::from_json<const char *>(__param["action_name"]);
    auto action_name = lhg::from_json_fix<const char *>(action_name_temp);
// LHG SEC END
    auto __return = MaaToolkitUnregisterCustomActionExecutor(
        handle,
        action_name
    );
// LHG SEC BEGIN lhg.impl.MaaToolkitUnregisterCustomActionExecutor.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitUnregisterCustomActionExecutor.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitFindWindow_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaToolkitFindWindow.arg.class_name.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["class_name"])) {
            __error = "class_name should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitFindWindow.arg.window_name.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["window_name"])) {
            __error = "window_name should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitFindWindow.arg.class_name
// LHG SEC DEF
    auto class_name_temp = lhg::from_json<const char *>(__param["class_name"]);
    auto class_name = lhg::from_json_fix<const char *>(class_name_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitFindWindow.arg.window_name
// LHG SEC DEF
    auto window_name_temp = lhg::from_json<const char *>(__param["window_name"]);
    auto window_name = lhg::from_json_fix<const char *>(window_name_temp);
// LHG SEC END
    auto __return = MaaToolkitFindWindow(
        class_name,
        window_name
    );
// LHG SEC BEGIN lhg.impl.MaaToolkitFindWindow.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitFindWindow.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitSearchWindow_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaToolkitSearchWindow.arg.class_name.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["class_name"])) {
            __error = "class_name should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitSearchWindow.arg.window_name.check
// LHG SEC DEF
    if constexpr (lhg::check_t<const char *>::enable) {
        if (!lhg::check_var<const char *>(__param["window_name"])) {
            __error = "window_name should be string";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitSearchWindow.arg.class_name
// LHG SEC DEF
    auto class_name_temp = lhg::from_json<const char *>(__param["class_name"]);
    auto class_name = lhg::from_json_fix<const char *>(class_name_temp);
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitSearchWindow.arg.window_name
// LHG SEC DEF
    auto window_name_temp = lhg::from_json<const char *>(__param["window_name"]);
    auto window_name = lhg::from_json_fix<const char *>(window_name_temp);
// LHG SEC END
    auto __return = MaaToolkitSearchWindow(
        class_name,
        window_name
    );
// LHG SEC BEGIN lhg.impl.MaaToolkitSearchWindow.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitSearchWindow.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitGetWindow_Wrapper(json::object __param, std::string &__error) {
// LHG SEC BEGIN lhg.impl.MaaToolkitGetWindow.arg.index.check
// LHG SEC DEF
    if constexpr (lhg::check_t<unsigned long long>::enable) {
        if (!lhg::check_var<unsigned long long>(__param["index"])) {
            __error = "index should be unsigned long long";
            return std::nullopt;
        }
    }
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitGetWindow.arg.index
// LHG SEC DEF
    auto index_temp = lhg::from_json<unsigned long long>(__param["index"]);
    auto index = lhg::from_json_fix<unsigned long long>(index_temp);
// LHG SEC END
    auto __return = MaaToolkitGetWindow(
        index
    );
// LHG SEC BEGIN lhg.impl.MaaToolkitGetWindow.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitGetWindow.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

std::optional<json::object> MaaToolkitGetCursorWindow_Wrapper(json::object __param, std::string &__error) {
    auto __return = MaaToolkitGetCursorWindow(

    );
// LHG SEC BEGIN lhg.impl.MaaToolkitGetCursorWindow.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
// LHG SEC BEGIN lhg.impl.MaaToolkitGetCursorWindow.final
// LHG SEC DEF
    return json::object { { "return", lhg::to_json(__ret) },  };
// LHG SEC END
}

bool handle_request(Context& ctx, UrlSegments segs) {
    auto obj = json::parse(ctx.req_.body()).value_or(json::object {}).as_object();

    // callback MaaAPICallback
    if (lhg::handle_callback("MaaAPICallback", MaaAPICallback__Manager, ctx, segs, obj, [](const auto& args) {
        auto v0 = std::get<0>(args);
        auto v1 = std::get<1>(args);
        return json::object {
            { "msg", v0 },
            { "details_json", v1 },
        };
    }, [](const auto& ret) {
        return 0;
    })) {
        return true;
    }

    // opaque MaaControllerAPI
    if (lhg::handle_opaque("MaaControllerAPI", MaaControllerAPI__OpaqueManager, ctx, segs, obj)) {
        return true;
    }

    // opaque MaaResourceAPI
    if (lhg::handle_opaque("MaaResourceAPI", MaaResourceAPI__OpaqueManager, ctx, segs, obj)) {
        return true;
    }

    // opaque MaaInstanceAPI
    if (lhg::handle_opaque("MaaInstanceAPI", MaaInstanceAPI__OpaqueManager, ctx, segs, obj)) {
        return true;
    }

    // opaque MaaImageBuffer
    if (lhg::handle_opaque("MaaImageBuffer", MaaImageBuffer__OpaqueManager, ctx, segs, obj)) {
        return true;
    }

    const static lhg::api_info_map wrappers = {
        { "MaaAdbControllerCreate", { &MaaAdbControllerCreate_Wrapper, &MaaAdbControllerCreate_HelperInput, &MaaAdbControllerCreate_HelperOutput } },
        { "MaaWin32ControllerCreate", { &MaaWin32ControllerCreate_Wrapper, &MaaWin32ControllerCreate_HelperInput, &MaaWin32ControllerCreate_HelperOutput } },
        { "MaaAdbControllerCreateV2", { &MaaAdbControllerCreateV2_Wrapper, &MaaAdbControllerCreateV2_HelperInput, &MaaAdbControllerCreateV2_HelperOutput } },
        { "MaaThriftControllerCreate", { &MaaThriftControllerCreate_Wrapper, &MaaThriftControllerCreate_HelperInput, &MaaThriftControllerCreate_HelperOutput } },
        { "MaaDbgControllerCreate", { &MaaDbgControllerCreate_Wrapper, &MaaDbgControllerCreate_HelperInput, &MaaDbgControllerCreate_HelperOutput } },
        { "MaaControllerDestroy", { &MaaControllerDestroy_Wrapper, &MaaControllerDestroy_HelperInput, &MaaControllerDestroy_HelperOutput } },
        { "MaaControllerSetOption", { &MaaControllerSetOption_Wrapper, &MaaControllerSetOption_HelperInput, &MaaControllerSetOption_HelperOutput } },
        { "MaaControllerPostConnection", { &MaaControllerPostConnection_Wrapper, &MaaControllerPostConnection_HelperInput, &MaaControllerPostConnection_HelperOutput } },
        { "MaaControllerPostClick", { &MaaControllerPostClick_Wrapper, &MaaControllerPostClick_HelperInput, &MaaControllerPostClick_HelperOutput } },
        { "MaaControllerPostSwipe", { &MaaControllerPostSwipe_Wrapper, &MaaControllerPostSwipe_HelperInput, &MaaControllerPostSwipe_HelperOutput } },
        { "MaaControllerPostPressKey", { &MaaControllerPostPressKey_Wrapper, &MaaControllerPostPressKey_HelperInput, &MaaControllerPostPressKey_HelperOutput } },
        { "MaaControllerPostInputText", { &MaaControllerPostInputText_Wrapper, &MaaControllerPostInputText_HelperInput, &MaaControllerPostInputText_HelperOutput } },
        { "MaaControllerPostTouchDown", { &MaaControllerPostTouchDown_Wrapper, &MaaControllerPostTouchDown_HelperInput, &MaaControllerPostTouchDown_HelperOutput } },
        { "MaaControllerPostTouchMove", { &MaaControllerPostTouchMove_Wrapper, &MaaControllerPostTouchMove_HelperInput, &MaaControllerPostTouchMove_HelperOutput } },
        { "MaaControllerPostTouchUp", { &MaaControllerPostTouchUp_Wrapper, &MaaControllerPostTouchUp_HelperInput, &MaaControllerPostTouchUp_HelperOutput } },
        { "MaaControllerPostScreencap", { &MaaControllerPostScreencap_Wrapper, &MaaControllerPostScreencap_HelperInput, &MaaControllerPostScreencap_HelperOutput } },
        { "MaaControllerStatus", { &MaaControllerStatus_Wrapper, &MaaControllerStatus_HelperInput, &MaaControllerStatus_HelperOutput } },
        { "MaaControllerWait", { &MaaControllerWait_Wrapper, &MaaControllerWait_HelperInput, &MaaControllerWait_HelperOutput } },
        { "MaaControllerConnected", { &MaaControllerConnected_Wrapper, &MaaControllerConnected_HelperInput, &MaaControllerConnected_HelperOutput } },
        { "MaaControllerGetImage", { &MaaControllerGetImage_Wrapper, &MaaControllerGetImage_HelperInput, &MaaControllerGetImage_HelperOutput } },
        { "MaaControllerGetUUID", { &MaaControllerGetUUID_Wrapper, &MaaControllerGetUUID_HelperInput, &MaaControllerGetUUID_HelperOutput } },
        { "MaaCreate", { &MaaCreate_Wrapper, &MaaCreate_HelperInput, &MaaCreate_HelperOutput } },
        { "MaaDestroy", { &MaaDestroy_Wrapper, &MaaDestroy_HelperInput, &MaaDestroy_HelperOutput } },
        { "MaaSetOption", { &MaaSetOption_Wrapper, &MaaSetOption_HelperInput, &MaaSetOption_HelperOutput } },
        { "MaaBindResource", { &MaaBindResource_Wrapper, &MaaBindResource_HelperInput, &MaaBindResource_HelperOutput } },
        { "MaaBindController", { &MaaBindController_Wrapper, &MaaBindController_HelperInput, &MaaBindController_HelperOutput } },
        { "MaaInited", { &MaaInited_Wrapper, &MaaInited_HelperInput, &MaaInited_HelperOutput } },
        { "MaaClearCustomRecognizer", { &MaaClearCustomRecognizer_Wrapper, &MaaClearCustomRecognizer_HelperInput, &MaaClearCustomRecognizer_HelperOutput } },
        { "MaaClearCustomAction", { &MaaClearCustomAction_Wrapper, &MaaClearCustomAction_HelperInput, &MaaClearCustomAction_HelperOutput } },
        { "MaaPostTask", { &MaaPostTask_Wrapper, &MaaPostTask_HelperInput, &MaaPostTask_HelperOutput } },
        { "MaaSetTaskParam", { &MaaSetTaskParam_Wrapper, &MaaSetTaskParam_HelperInput, &MaaSetTaskParam_HelperOutput } },
        { "MaaTaskStatus", { &MaaTaskStatus_Wrapper, &MaaTaskStatus_HelperInput, &MaaTaskStatus_HelperOutput } },
        { "MaaWaitTask", { &MaaWaitTask_Wrapper, &MaaWaitTask_HelperInput, &MaaWaitTask_HelperOutput } },
        { "MaaTaskAllFinished", { &MaaTaskAllFinished_Wrapper, &MaaTaskAllFinished_HelperInput, &MaaTaskAllFinished_HelperOutput } },
        { "MaaPostStop", { &MaaPostStop_Wrapper, &MaaPostStop_HelperInput, &MaaPostStop_HelperOutput } },
        { "MaaStop", { &MaaStop_Wrapper, &MaaStop_HelperInput, &MaaStop_HelperOutput } },
        { "MaaGetResource", { &MaaGetResource_Wrapper, &MaaGetResource_HelperInput, &MaaGetResource_HelperOutput } },
        { "MaaGetController", { &MaaGetController_Wrapper, &MaaGetController_HelperInput, &MaaGetController_HelperOutput } },
        { "MaaResourceCreate", { &MaaResourceCreate_Wrapper, &MaaResourceCreate_HelperInput, &MaaResourceCreate_HelperOutput } },
        { "MaaResourceDestroy", { &MaaResourceDestroy_Wrapper, &MaaResourceDestroy_HelperInput, &MaaResourceDestroy_HelperOutput } },
        { "MaaResourcePostPath", { &MaaResourcePostPath_Wrapper, &MaaResourcePostPath_HelperInput, &MaaResourcePostPath_HelperOutput } },
        { "MaaResourceStatus", { &MaaResourceStatus_Wrapper, &MaaResourceStatus_HelperInput, &MaaResourceStatus_HelperOutput } },
        { "MaaResourceWait", { &MaaResourceWait_Wrapper, &MaaResourceWait_HelperInput, &MaaResourceWait_HelperOutput } },
        { "MaaResourceLoaded", { &MaaResourceLoaded_Wrapper, &MaaResourceLoaded_HelperInput, &MaaResourceLoaded_HelperOutput } },
        { "MaaResourceSetOption", { &MaaResourceSetOption_Wrapper, &MaaResourceSetOption_HelperInput, &MaaResourceSetOption_HelperOutput } },
        { "MaaResourceGetHash", { &MaaResourceGetHash_Wrapper, &MaaResourceGetHash_HelperInput, &MaaResourceGetHash_HelperOutput } },
        { "MaaResourceGetTaskList", { &MaaResourceGetTaskList_Wrapper, &MaaResourceGetTaskList_HelperInput, &MaaResourceGetTaskList_HelperOutput } },
        { "MaaCreateImageBuffer", { &MaaCreateImageBuffer_Wrapper, &MaaCreateImageBuffer_HelperInput, &MaaCreateImageBuffer_HelperOutput } },
        { "MaaDestroyImageBuffer", { &MaaDestroyImageBuffer_Wrapper, &MaaDestroyImageBuffer_HelperInput, &MaaDestroyImageBuffer_HelperOutput } },
        { "MaaIsImageEmpty", { &MaaIsImageEmpty_Wrapper, &MaaIsImageEmpty_HelperInput, &MaaIsImageEmpty_HelperOutput } },
        { "MaaClearImage", { &MaaClearImage_Wrapper, &MaaClearImage_HelperInput, &MaaClearImage_HelperOutput } },
        { "MaaGetImageWidth", { &MaaGetImageWidth_Wrapper, &MaaGetImageWidth_HelperInput, &MaaGetImageWidth_HelperOutput } },
        { "MaaGetImageHeight", { &MaaGetImageHeight_Wrapper, &MaaGetImageHeight_HelperInput, &MaaGetImageHeight_HelperOutput } },
        { "MaaGetImageType", { &MaaGetImageType_Wrapper, &MaaGetImageType_HelperInput, &MaaGetImageType_HelperOutput } },
        { "MaaGetImageEncoded", { &MaaGetImageEncoded_Wrapper, &MaaGetImageEncoded_HelperInput, &MaaGetImageEncoded_HelperOutput } },
        { "MaaSetImageEncoded", { &MaaSetImageEncoded_Wrapper, &MaaSetImageEncoded_HelperInput, &MaaSetImageEncoded_HelperOutput } },
        { "MaaVersion", { &MaaVersion_Wrapper, &MaaVersion_HelperInput, &MaaVersion_HelperOutput } },
        { "MaaSetGlobalOption", { &MaaSetGlobalOption_Wrapper, &MaaSetGlobalOption_HelperInput, &MaaSetGlobalOption_HelperOutput } },
        { "MaaToolkitInit", { &MaaToolkitInit_Wrapper, &MaaToolkitInit_HelperInput, &MaaToolkitInit_HelperOutput } },
        { "MaaToolkitUninit", { &MaaToolkitUninit_Wrapper, &MaaToolkitUninit_HelperInput, &MaaToolkitUninit_HelperOutput } },
        { "MaaToolkitFindDevice", { &MaaToolkitFindDevice_Wrapper, &MaaToolkitFindDevice_HelperInput, &MaaToolkitFindDevice_HelperOutput } },
        { "MaaToolkitFindDeviceWithAdb", { &MaaToolkitFindDeviceWithAdb_Wrapper, &MaaToolkitFindDeviceWithAdb_HelperInput, &MaaToolkitFindDeviceWithAdb_HelperOutput } },
        { "MaaToolkitPostFindDevice", { &MaaToolkitPostFindDevice_Wrapper, &MaaToolkitPostFindDevice_HelperInput, &MaaToolkitPostFindDevice_HelperOutput } },
        { "MaaToolkitPostFindDeviceWithAdb", { &MaaToolkitPostFindDeviceWithAdb_Wrapper, &MaaToolkitPostFindDeviceWithAdb_HelperInput, &MaaToolkitPostFindDeviceWithAdb_HelperOutput } },
        { "MaaToolkitIsFindDeviceCompleted", { &MaaToolkitIsFindDeviceCompleted_Wrapper, &MaaToolkitIsFindDeviceCompleted_HelperInput, &MaaToolkitIsFindDeviceCompleted_HelperOutput } },
        { "MaaToolkitWaitForFindDeviceToComplete", { &MaaToolkitWaitForFindDeviceToComplete_Wrapper, &MaaToolkitWaitForFindDeviceToComplete_HelperInput, &MaaToolkitWaitForFindDeviceToComplete_HelperOutput } },
        { "MaaToolkitGetDeviceCount", { &MaaToolkitGetDeviceCount_Wrapper, &MaaToolkitGetDeviceCount_HelperInput, &MaaToolkitGetDeviceCount_HelperOutput } },
        { "MaaToolkitGetDeviceName", { &MaaToolkitGetDeviceName_Wrapper, &MaaToolkitGetDeviceName_HelperInput, &MaaToolkitGetDeviceName_HelperOutput } },
        { "MaaToolkitGetDeviceAdbPath", { &MaaToolkitGetDeviceAdbPath_Wrapper, &MaaToolkitGetDeviceAdbPath_HelperInput, &MaaToolkitGetDeviceAdbPath_HelperOutput } },
        { "MaaToolkitGetDeviceAdbSerial", { &MaaToolkitGetDeviceAdbSerial_Wrapper, &MaaToolkitGetDeviceAdbSerial_HelperInput, &MaaToolkitGetDeviceAdbSerial_HelperOutput } },
        { "MaaToolkitGetDeviceAdbControllerType", { &MaaToolkitGetDeviceAdbControllerType_Wrapper, &MaaToolkitGetDeviceAdbControllerType_HelperInput, &MaaToolkitGetDeviceAdbControllerType_HelperOutput } },
        { "MaaToolkitGetDeviceAdbConfig", { &MaaToolkitGetDeviceAdbConfig_Wrapper, &MaaToolkitGetDeviceAdbConfig_HelperInput, &MaaToolkitGetDeviceAdbConfig_HelperOutput } },
        { "MaaToolkitRegisterCustomRecognizerExecutor", { &MaaToolkitRegisterCustomRecognizerExecutor_Wrapper, &MaaToolkitRegisterCustomRecognizerExecutor_HelperInput, &MaaToolkitRegisterCustomRecognizerExecutor_HelperOutput } },
        { "MaaToolkitUnregisterCustomRecognizerExecutor", { &MaaToolkitUnregisterCustomRecognizerExecutor_Wrapper, &MaaToolkitUnregisterCustomRecognizerExecutor_HelperInput, &MaaToolkitUnregisterCustomRecognizerExecutor_HelperOutput } },
        { "MaaToolkitRegisterCustomActionExecutor", { &MaaToolkitRegisterCustomActionExecutor_Wrapper, &MaaToolkitRegisterCustomActionExecutor_HelperInput, &MaaToolkitRegisterCustomActionExecutor_HelperOutput } },
        { "MaaToolkitUnregisterCustomActionExecutor", { &MaaToolkitUnregisterCustomActionExecutor_Wrapper, &MaaToolkitUnregisterCustomActionExecutor_HelperInput, &MaaToolkitUnregisterCustomActionExecutor_HelperOutput } },
        { "MaaToolkitFindWindow", { &MaaToolkitFindWindow_Wrapper, &MaaToolkitFindWindow_HelperInput, &MaaToolkitFindWindow_HelperOutput } },
        { "MaaToolkitSearchWindow", { &MaaToolkitSearchWindow_Wrapper, &MaaToolkitSearchWindow_HelperInput, &MaaToolkitSearchWindow_HelperOutput } },
        { "MaaToolkitGetWindow", { &MaaToolkitGetWindow_Wrapper, &MaaToolkitGetWindow_HelperInput, &MaaToolkitGetWindow_HelperOutput } },
        { "MaaToolkitGetCursorWindow", { &MaaToolkitGetCursorWindow_Wrapper, &MaaToolkitGetCursorWindow_HelperInput, &MaaToolkitGetCursorWindow_HelperOutput } },
    };
    if (lhg::handle_api(ctx, segs, obj, wrappers)) {
        return true;
    }

    if (lhg::handle_help(ctx, segs, wrappers, { "MaaControllerAPI", "MaaResourceAPI", "MaaInstanceAPI", "MaaImageBuffer" }, [](json::object& result) {
            // MaaAPICallback
            result["/callback/MaaAPICallback/add"] = json::object { { "body", json::object {} }, { "response", { { "data", { { "id", "string" } } } } } };
            result["/callback/MaaAPICallback/:id/del"] = json::object { { "body", json::object {} }, { "response", { { "data", json::object {} }, { "error", "string" } } } };
            result["/callback/MaaAPICallback/:id/pull"] = json::object { { "body", json::object {} }, { "response", { { "data", { { "ids", "string[]" } } } } } };
            result["/callback/MaaAPICallback/:id/:cid/request"] = { { "body", json::object {} }, { "response", { { "data", json::object {
                { "msg", lhg::schema_t<decltype(std::get<0>(lhg::callback_manager<void (*)(const char *, const char *, void *)>::CallbackContext::args_type {}))>::schema },
                { "details_json", lhg::schema_t<decltype(std::get<1>(lhg::callback_manager<void (*)(const char *, const char *, void *)>::CallbackContext::args_type {}))>::schema },
            } }, { "error", "string" } } } };
            result["/callback/MaaAPICallback/:id/:cid/response"] = json::object { { "body", json::object {} }, { "response", { { "data", json::object {} }, { "error", "string" } } } };

    })) {
        return true;
    }

    return false;
}
