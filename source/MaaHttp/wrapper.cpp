// clang-format off

#include "LHGUtils.h"
#define LHG_PROCESS

// LHG SEC BEGIN lhg.include
#include "include.h"
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

struct __MaaAdbControllerCreate_t {
    struct __adb_path_t {
        using type = const char *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "adb_path";
    };
    struct __address_t {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "address";
    };
    struct __type_t {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "type";
    };
    struct __config_t {
        using type = const char *;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "config";
    };
    struct __callback_t {
        using type = void (*)(const char *, const char *, void *);
        static constexpr size_t index = 4;
        static constexpr const char* const name = "callback";
    };
    struct __callback_arg_t {
        using type = void *;
        static constexpr size_t index = 5;
        static constexpr const char* const name = "callback_arg";
    };
    using arguments_t = std::tuple<__adb_path_t, __address_t, __type_t, __config_t, __callback_t, __callback_arg_t>;
    using return_t = MaaControllerAPI *;
};

template<>
struct lhg::is_callback<__MaaAdbControllerCreate_t::__callback_t>
{
    static constexpr const bool value = true;
    static constexpr const char* const name = "MaaAPICallback";
    static constexpr const size_t context = 2;
    using traits = lhg::func_traits<typename __MaaAdbControllerCreate_t::__callback_t::type>;
    static decltype(MaaAPICallback__Manager) &manager; 
};
decltype(MaaAPICallback__Manager)& lhg::is_callback<__MaaAdbControllerCreate_t::__callback_t>::manager = MaaAPICallback__Manager;

template<>
struct lhg::is_callback_context<__MaaAdbControllerCreate_t::__callback_arg_t>
{
    static constexpr const bool value = true;
    using callback_arg_tag = __MaaAdbControllerCreate_t::__callback_t;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaAdbControllerCreate.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaWin32ControllerCreate_t {
    struct __hWnd_t {
        using type = unsigned long long;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "hWnd";
    };
    struct __type_t {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "type";
    };
    struct __callback_t {
        using type = void (*)(const char *, const char *, void *);
        static constexpr size_t index = 2;
        static constexpr const char* const name = "callback";
    };
    struct __callback_arg_t {
        using type = void *;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "callback_arg";
    };
    using arguments_t = std::tuple<__hWnd_t, __type_t, __callback_t, __callback_arg_t>;
    using return_t = MaaControllerAPI *;
};

template<>
struct lhg::is_callback<__MaaWin32ControllerCreate_t::__callback_t>
{
    static constexpr const bool value = true;
    static constexpr const char* const name = "MaaAPICallback";
    static constexpr const size_t context = 2;
    using traits = lhg::func_traits<typename __MaaWin32ControllerCreate_t::__callback_t::type>;
    static decltype(MaaAPICallback__Manager) &manager; 
};
decltype(MaaAPICallback__Manager)& lhg::is_callback<__MaaWin32ControllerCreate_t::__callback_t>::manager = MaaAPICallback__Manager;

template<>
struct lhg::is_callback_context<__MaaWin32ControllerCreate_t::__callback_arg_t>
{
    static constexpr const bool value = true;
    using callback_arg_tag = __MaaWin32ControllerCreate_t::__callback_t;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaWin32ControllerCreate.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaAdbControllerCreateV2_t {
    struct __adb_path_t {
        using type = const char *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "adb_path";
    };
    struct __address_t {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "address";
    };
    struct __type_t {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "type";
    };
    struct __config_t {
        using type = const char *;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "config";
    };
    struct __agent_path_t {
        using type = const char *;
        static constexpr size_t index = 4;
        static constexpr const char* const name = "agent_path";
    };
    struct __callback_t {
        using type = void (*)(const char *, const char *, void *);
        static constexpr size_t index = 5;
        static constexpr const char* const name = "callback";
    };
    struct __callback_arg_t {
        using type = void *;
        static constexpr size_t index = 6;
        static constexpr const char* const name = "callback_arg";
    };
    using arguments_t = std::tuple<__adb_path_t, __address_t, __type_t, __config_t, __agent_path_t, __callback_t, __callback_arg_t>;
    using return_t = MaaControllerAPI *;
};

template<>
struct lhg::is_callback<__MaaAdbControllerCreateV2_t::__callback_t>
{
    static constexpr const bool value = true;
    static constexpr const char* const name = "MaaAPICallback";
    static constexpr const size_t context = 2;
    using traits = lhg::func_traits<typename __MaaAdbControllerCreateV2_t::__callback_t::type>;
    static decltype(MaaAPICallback__Manager) &manager; 
};
decltype(MaaAPICallback__Manager)& lhg::is_callback<__MaaAdbControllerCreateV2_t::__callback_t>::manager = MaaAPICallback__Manager;

template<>
struct lhg::is_callback_context<__MaaAdbControllerCreateV2_t::__callback_arg_t>
{
    static constexpr const bool value = true;
    using callback_arg_tag = __MaaAdbControllerCreateV2_t::__callback_t;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaAdbControllerCreateV2.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaThriftControllerCreate_t {
    struct __type_t {
        using type = int;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "type";
    };
    struct __host_t {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "host";
    };
    struct __port_t {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "port";
    };
    struct __config_t {
        using type = const char *;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "config";
    };
    struct __callback_t {
        using type = void (*)(const char *, const char *, void *);
        static constexpr size_t index = 4;
        static constexpr const char* const name = "callback";
    };
    struct __callback_arg_t {
        using type = void *;
        static constexpr size_t index = 5;
        static constexpr const char* const name = "callback_arg";
    };
    using arguments_t = std::tuple<__type_t, __host_t, __port_t, __config_t, __callback_t, __callback_arg_t>;
    using return_t = MaaControllerAPI *;
};

template<>
struct lhg::is_callback<__MaaThriftControllerCreate_t::__callback_t>
{
    static constexpr const bool value = true;
    static constexpr const char* const name = "MaaAPICallback";
    static constexpr const size_t context = 2;
    using traits = lhg::func_traits<typename __MaaThriftControllerCreate_t::__callback_t::type>;
    static decltype(MaaAPICallback__Manager) &manager; 
};
decltype(MaaAPICallback__Manager)& lhg::is_callback<__MaaThriftControllerCreate_t::__callback_t>::manager = MaaAPICallback__Manager;

template<>
struct lhg::is_callback_context<__MaaThriftControllerCreate_t::__callback_arg_t>
{
    static constexpr const bool value = true;
    using callback_arg_tag = __MaaThriftControllerCreate_t::__callback_t;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaThriftControllerCreate.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaDbgControllerCreate_t {
    struct __read_path_t {
        using type = const char *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "read_path";
    };
    struct __write_path_t {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "write_path";
    };
    struct __type_t {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "type";
    };
    struct __config_t {
        using type = const char *;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "config";
    };
    struct __callback_t {
        using type = void (*)(const char *, const char *, void *);
        static constexpr size_t index = 4;
        static constexpr const char* const name = "callback";
    };
    struct __callback_arg_t {
        using type = void *;
        static constexpr size_t index = 5;
        static constexpr const char* const name = "callback_arg";
    };
    using arguments_t = std::tuple<__read_path_t, __write_path_t, __type_t, __config_t, __callback_t, __callback_arg_t>;
    using return_t = MaaControllerAPI *;
};

template<>
struct lhg::is_callback<__MaaDbgControllerCreate_t::__callback_t>
{
    static constexpr const bool value = true;
    static constexpr const char* const name = "MaaAPICallback";
    static constexpr const size_t context = 2;
    using traits = lhg::func_traits<typename __MaaDbgControllerCreate_t::__callback_t::type>;
    static decltype(MaaAPICallback__Manager) &manager; 
};
decltype(MaaAPICallback__Manager)& lhg::is_callback<__MaaDbgControllerCreate_t::__callback_t>::manager = MaaAPICallback__Manager;

template<>
struct lhg::is_callback_context<__MaaDbgControllerCreate_t::__callback_arg_t>
{
    static constexpr const bool value = true;
    using callback_arg_tag = __MaaDbgControllerCreate_t::__callback_t;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaDbgControllerCreate.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerDestroy_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    using arguments_t = std::tuple<__ctrl_t>;
    using return_t = void;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerDestroy.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerPostConnection_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    using arguments_t = std::tuple<__ctrl_t>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostConnection.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerPostClick_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct __x_t {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "x";
    };
    struct __y_t {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "y";
    };
    using arguments_t = std::tuple<__ctrl_t, __x_t, __y_t>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostClick.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerPostSwipe_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct __x1_t {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "x1";
    };
    struct __y1_t {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "y1";
    };
    struct __x2_t {
        using type = int;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "x2";
    };
    struct __y2_t {
        using type = int;
        static constexpr size_t index = 4;
        static constexpr const char* const name = "y2";
    };
    struct __duration_t {
        using type = int;
        static constexpr size_t index = 5;
        static constexpr const char* const name = "duration";
    };
    using arguments_t = std::tuple<__ctrl_t, __x1_t, __y1_t, __x2_t, __y2_t, __duration_t>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostSwipe.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerPostPressKey_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct __keycode_t {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "keycode";
    };
    using arguments_t = std::tuple<__ctrl_t, __keycode_t>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostPressKey.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerPostInputText_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct __text_t {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "text";
    };
    using arguments_t = std::tuple<__ctrl_t, __text_t>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostInputText.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerPostTouchDown_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct __contact_t {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "contact";
    };
    struct __x_t {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "x";
    };
    struct __y_t {
        using type = int;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "y";
    };
    struct __pressure_t {
        using type = int;
        static constexpr size_t index = 4;
        static constexpr const char* const name = "pressure";
    };
    using arguments_t = std::tuple<__ctrl_t, __contact_t, __x_t, __y_t, __pressure_t>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostTouchDown.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerPostTouchMove_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct __contact_t {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "contact";
    };
    struct __x_t {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "x";
    };
    struct __y_t {
        using type = int;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "y";
    };
    struct __pressure_t {
        using type = int;
        static constexpr size_t index = 4;
        static constexpr const char* const name = "pressure";
    };
    using arguments_t = std::tuple<__ctrl_t, __contact_t, __x_t, __y_t, __pressure_t>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostTouchMove.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerPostTouchUp_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct __contact_t {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "contact";
    };
    using arguments_t = std::tuple<__ctrl_t, __contact_t>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostTouchUp.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerPostScreencap_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    using arguments_t = std::tuple<__ctrl_t>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostScreencap.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerStatus_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct __id_t {
        using type = long long;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "id";
    };
    using arguments_t = std::tuple<__ctrl_t, __id_t>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerStatus.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerWait_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct __id_t {
        using type = long long;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "id";
    };
    using arguments_t = std::tuple<__ctrl_t, __id_t>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerWait.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerConnected_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    using arguments_t = std::tuple<__ctrl_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerConnected.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerGetImage_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct __buffer_t {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "buffer";
    };
    using arguments_t = std::tuple<__ctrl_t, __buffer_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerGetImage.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerGetUUID_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct __buffer_t {
        using type = MaaStringBuffer *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "buffer";
    };
    using arguments_t = std::tuple<__ctrl_t, __buffer_t>;
    using return_t = unsigned char;
};

template<>
struct lhg::is_output<__MaaControllerGetUUID_t::__buffer_t>
{
    static constexpr const bool value = true;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerGetUUID.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaCreate_t {
    struct __callback_t {
        using type = void (*)(const char *, const char *, void *);
        static constexpr size_t index = 0;
        static constexpr const char* const name = "callback";
    };
    struct __callback_arg_t {
        using type = void *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "callback_arg";
    };
    using arguments_t = std::tuple<__callback_t, __callback_arg_t>;
    using return_t = MaaInstanceAPI *;
};

template<>
struct lhg::is_callback<__MaaCreate_t::__callback_t>
{
    static constexpr const bool value = true;
    static constexpr const char* const name = "MaaAPICallback";
    static constexpr const size_t context = 2;
    using traits = lhg::func_traits<typename __MaaCreate_t::__callback_t::type>;
    static decltype(MaaAPICallback__Manager) &manager; 
};
decltype(MaaAPICallback__Manager)& lhg::is_callback<__MaaCreate_t::__callback_t>::manager = MaaAPICallback__Manager;

template<>
struct lhg::is_callback_context<__MaaCreate_t::__callback_arg_t>
{
    static constexpr const bool value = true;
    using callback_arg_tag = __MaaCreate_t::__callback_t;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaCreate.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaDestroy_t {
    struct __inst_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<__inst_t>;
    using return_t = void;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaDestroy.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaBindResource_t {
    struct __inst_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    struct __res_t {
        using type = MaaResourceAPI *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "res";
    };
    using arguments_t = std::tuple<__inst_t, __res_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaBindResource.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaBindController_t {
    struct __inst_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "ctrl";
    };
    using arguments_t = std::tuple<__inst_t, __ctrl_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaBindController.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaInited_t {
    struct __inst_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<__inst_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaInited.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaClearCustomRecognizer_t {
    struct __inst_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<__inst_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaClearCustomRecognizer.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaClearCustomAction_t {
    struct __inst_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<__inst_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaClearCustomAction.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaPostTask_t {
    struct __inst_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    struct __entry_t {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "entry";
    };
    struct __param_t {
        using type = const char *;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "param";
    };
    using arguments_t = std::tuple<__inst_t, __entry_t, __param_t>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaPostTask.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaSetTaskParam_t {
    struct __inst_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    struct __id_t {
        using type = long long;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "id";
    };
    struct __param_t {
        using type = const char *;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "param";
    };
    using arguments_t = std::tuple<__inst_t, __id_t, __param_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaSetTaskParam.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaTaskStatus_t {
    struct __inst_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    struct __id_t {
        using type = long long;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "id";
    };
    using arguments_t = std::tuple<__inst_t, __id_t>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaTaskStatus.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaWaitTask_t {
    struct __inst_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    struct __id_t {
        using type = long long;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "id";
    };
    using arguments_t = std::tuple<__inst_t, __id_t>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaWaitTask.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaTaskAllFinished_t {
    struct __inst_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<__inst_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaTaskAllFinished.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaPostStop_t {
    struct __inst_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<__inst_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaPostStop.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaStop_t {
    struct __inst_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<__inst_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaStop.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaGetResource_t {
    struct __inst_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<__inst_t>;
    using return_t = MaaResourceAPI *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaGetResource.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaGetController_t {
    struct __inst_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<__inst_t>;
    using return_t = MaaControllerAPI *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaGetController.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaResourceCreate_t {
    struct __callback_t {
        using type = void (*)(const char *, const char *, void *);
        static constexpr size_t index = 0;
        static constexpr const char* const name = "callback";
    };
    struct __callback_arg_t {
        using type = void *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "callback_arg";
    };
    using arguments_t = std::tuple<__callback_t, __callback_arg_t>;
    using return_t = MaaResourceAPI *;
};

template<>
struct lhg::is_callback<__MaaResourceCreate_t::__callback_t>
{
    static constexpr const bool value = true;
    static constexpr const char* const name = "MaaAPICallback";
    static constexpr const size_t context = 2;
    using traits = lhg::func_traits<typename __MaaResourceCreate_t::__callback_t::type>;
    static decltype(MaaAPICallback__Manager) &manager; 
};
decltype(MaaAPICallback__Manager)& lhg::is_callback<__MaaResourceCreate_t::__callback_t>::manager = MaaAPICallback__Manager;

template<>
struct lhg::is_callback_context<__MaaResourceCreate_t::__callback_arg_t>
{
    static constexpr const bool value = true;
    using callback_arg_tag = __MaaResourceCreate_t::__callback_t;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaResourceCreate.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaResourceDestroy_t {
    struct __res_t {
        using type = MaaResourceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "res";
    };
    using arguments_t = std::tuple<__res_t>;
    using return_t = void;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaResourceDestroy.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaResourcePostPath_t {
    struct __res_t {
        using type = MaaResourceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "res";
    };
    struct __path_t {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "path";
    };
    using arguments_t = std::tuple<__res_t, __path_t>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaResourcePostPath.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaResourceStatus_t {
    struct __res_t {
        using type = MaaResourceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "res";
    };
    struct __id_t {
        using type = long long;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "id";
    };
    using arguments_t = std::tuple<__res_t, __id_t>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaResourceStatus.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaResourceWait_t {
    struct __res_t {
        using type = MaaResourceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "res";
    };
    struct __id_t {
        using type = long long;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "id";
    };
    using arguments_t = std::tuple<__res_t, __id_t>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaResourceWait.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaResourceLoaded_t {
    struct __res_t {
        using type = MaaResourceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "res";
    };
    using arguments_t = std::tuple<__res_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaResourceLoaded.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaResourceGetHash_t {
    struct __res_t {
        using type = MaaResourceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "res";
    };
    struct __buffer_t {
        using type = MaaStringBuffer *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "buffer";
    };
    using arguments_t = std::tuple<__res_t, __buffer_t>;
    using return_t = unsigned char;
};

template<>
struct lhg::is_output<__MaaResourceGetHash_t::__buffer_t>
{
    static constexpr const bool value = true;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaResourceGetHash.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaResourceGetTaskList_t {
    struct __res_t {
        using type = MaaResourceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "res";
    };
    struct __buffer_t {
        using type = MaaStringBuffer *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "buffer";
    };
    using arguments_t = std::tuple<__res_t, __buffer_t>;
    using return_t = unsigned char;
};

template<>
struct lhg::is_output<__MaaResourceGetTaskList_t::__buffer_t>
{
    static constexpr const bool value = true;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaResourceGetTaskList.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaCreateImageBuffer_t {
    using arguments_t = void;
    using return_t = MaaImageBuffer *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaCreateImageBuffer.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaDestroyImageBuffer_t {
    struct __handle_t {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    using arguments_t = std::tuple<__handle_t>;
    using return_t = void;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaDestroyImageBuffer.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaIsImageEmpty_t {
    struct __handle_t {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    using arguments_t = std::tuple<__handle_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaIsImageEmpty.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaClearImage_t {
    struct __handle_t {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    using arguments_t = std::tuple<__handle_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaClearImage.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaGetImageWidth_t {
    struct __handle_t {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    using arguments_t = std::tuple<__handle_t>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaGetImageWidth.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaGetImageHeight_t {
    struct __handle_t {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    using arguments_t = std::tuple<__handle_t>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaGetImageHeight.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaGetImageType_t {
    struct __handle_t {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    using arguments_t = std::tuple<__handle_t>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaGetImageType.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaGetImageEncoded_t {
    struct __handle_t {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    using arguments_t = std::tuple<__handle_t>;
    using return_t = unsigned char *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaGetImageEncoded.tag
template<>
struct ret_schema<__MaaGetImageEncoded_t>
{
    static constexpr const char* const schema = "string@buffer";
};
// LHG SEC END
}

struct __MaaSetImageEncoded_t {
    struct __handle_t {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    struct __data_t {
        using type = unsigned char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "data";
    };
    struct __size_t {
        using type = unsigned long long;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "size";
    };
    using arguments_t = std::tuple<__handle_t, __data_t, __size_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaSetImageEncoded.tag
template<>
struct arg_schema<__MaaSetImageEncoded_t::__data_t>
{
    static constexpr const char *const schema = "string@buffer";
};

template<>
struct shown_in_schema<__MaaSetImageEncoded_t::__size_t>
{
    static constexpr const bool value = false;
};
// LHG SEC END
}

struct __MaaVersion_t {
    using arguments_t = void;
    using return_t = const char *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaVersion.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitInit_t {
    using arguments_t = void;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitInit.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitUninit_t {
    using arguments_t = void;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitUninit.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitFindDevice_t {
    using arguments_t = void;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitFindDevice.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitFindDeviceWithAdb_t {
    struct __adb_path_t {
        using type = const char *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "adb_path";
    };
    using arguments_t = std::tuple<__adb_path_t>;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitFindDeviceWithAdb.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitPostFindDevice_t {
    using arguments_t = void;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitPostFindDevice.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitPostFindDeviceWithAdb_t {
    struct __adb_path_t {
        using type = const char *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "adb_path";
    };
    using arguments_t = std::tuple<__adb_path_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitPostFindDeviceWithAdb.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitIsFindDeviceCompleted_t {
    using arguments_t = void;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitIsFindDeviceCompleted.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitWaitForFindDeviceToComplete_t {
    using arguments_t = void;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitWaitForFindDeviceToComplete.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitGetDeviceCount_t {
    using arguments_t = void;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceCount.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitGetDeviceName_t {
    struct __index_t {
        using type = unsigned long long;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "index";
    };
    using arguments_t = std::tuple<__index_t>;
    using return_t = const char *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceName.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitGetDeviceAdbPath_t {
    struct __index_t {
        using type = unsigned long long;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "index";
    };
    using arguments_t = std::tuple<__index_t>;
    using return_t = const char *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceAdbPath.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitGetDeviceAdbSerial_t {
    struct __index_t {
        using type = unsigned long long;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "index";
    };
    using arguments_t = std::tuple<__index_t>;
    using return_t = const char *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceAdbSerial.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitGetDeviceAdbControllerType_t {
    struct __index_t {
        using type = unsigned long long;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "index";
    };
    using arguments_t = std::tuple<__index_t>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceAdbControllerType.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitGetDeviceAdbConfig_t {
    struct __index_t {
        using type = unsigned long long;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "index";
    };
    using arguments_t = std::tuple<__index_t>;
    using return_t = const char *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceAdbConfig.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitRegisterCustomRecognizerExecutor_t {
    struct __handle_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    struct __recognizer_name_t {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "recognizer_name";
    };
    struct __recognizer_exec_path_t {
        using type = const char *;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "recognizer_exec_path";
    };
    struct __recognizer_exec_param_json_t {
        using type = const char *;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "recognizer_exec_param_json";
    };
    using arguments_t = std::tuple<__handle_t, __recognizer_name_t, __recognizer_exec_path_t, __recognizer_exec_param_json_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitRegisterCustomRecognizerExecutor.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitUnregisterCustomRecognizerExecutor_t {
    struct __handle_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    struct __recognizer_name_t {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "recognizer_name";
    };
    using arguments_t = std::tuple<__handle_t, __recognizer_name_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitUnregisterCustomRecognizerExecutor.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitRegisterCustomActionExecutor_t {
    struct __handle_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    struct __action_name_t {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "action_name";
    };
    struct __action_exec_path_t {
        using type = const char *;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "action_exec_path";
    };
    struct __action_exec_param_json_t {
        using type = const char *;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "action_exec_param_json";
    };
    using arguments_t = std::tuple<__handle_t, __action_name_t, __action_exec_path_t, __action_exec_param_json_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitRegisterCustomActionExecutor.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitUnregisterCustomActionExecutor_t {
    struct __handle_t {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    struct __action_name_t {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "action_name";
    };
    using arguments_t = std::tuple<__handle_t, __action_name_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitUnregisterCustomActionExecutor.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitFindWindow_t {
    struct __class_name_t {
        using type = const char *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "class_name";
    };
    struct __window_name_t {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "window_name";
    };
    using arguments_t = std::tuple<__class_name_t, __window_name_t>;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitFindWindow.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitSearchWindow_t {
    struct __class_name_t {
        using type = const char *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "class_name";
    };
    struct __window_name_t {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "window_name";
    };
    using arguments_t = std::tuple<__class_name_t, __window_name_t>;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitSearchWindow.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitGetWindow_t {
    struct __index_t {
        using type = unsigned long long;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "index";
    };
    using arguments_t = std::tuple<__index_t>;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetWindow.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaToolkitGetCursorWindow_t {
    using arguments_t = void;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetCursorWindow.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerSetOptionString_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct __key_t {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "key";
    };
    struct __value_t {
        using type = const char *;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "value";
    };
    using arguments_t = std::tuple<__ctrl_t, __key_t, __value_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerSetOptionString.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerSetOptionInteger_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct __key_t {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "key";
    };
    struct __value_t {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "value";
    };
    using arguments_t = std::tuple<__ctrl_t, __key_t, __value_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerSetOptionInteger.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaControllerSetOptionBoolean_t {
    struct __ctrl_t {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct __key_t {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "key";
    };
    struct __value_t {
        using type = bool;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "value";
    };
    using arguments_t = std::tuple<__ctrl_t, __key_t, __value_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerSetOptionBoolean.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaSetGlobalOptionString_t {
    struct __key_t {
        using type = int;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "key";
    };
    struct __value_t {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "value";
    };
    using arguments_t = std::tuple<__key_t, __value_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaSetGlobalOptionString.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaSetGlobalOptionInteger_t {
    struct __key_t {
        using type = int;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "key";
    };
    struct __value_t {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "value";
    };
    using arguments_t = std::tuple<__key_t, __value_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaSetGlobalOptionInteger.tag
// LHG SEC DEF

// LHG SEC END
}

struct __MaaSetGlobalOptionBoolean_t {
    struct __key_t {
        using type = int;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "key";
    };
    struct __value_t {
        using type = bool;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "value";
    };
    using arguments_t = std::tuple<__key_t, __value_t>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaSetGlobalOptionBoolean.tag
// LHG SEC DEF

// LHG SEC END
}

static lhg::opaque_manager<MaaControllerAPI *> MaaControllerAPI__OpaqueManager;

template<>
struct lhg::schema_t<MaaControllerAPI *>
{
    static constexpr const char* const schema = "string@MaaControllerAPI";
};

template<>
struct lhg::is_opaque<MaaControllerAPI *> {
    static constexpr const bool value = true;
    using type = MaaControllerAPI;
    static constexpr const char* const name = "MaaControllerAPI";
    static lhg::opaque_manager<MaaControllerAPI *>& manager;
};
lhg::opaque_manager<MaaControllerAPI *>& lhg::is_opaque<MaaControllerAPI *>::manager = MaaControllerAPI__OpaqueManager;

template<>
struct lhg::is_opaque_free<MaaControllerAPI *, __MaaControllerDestroy_t> {
    static constexpr const bool value = true;
};

template<>
struct lhg::is_opaque_non_alloc<MaaControllerAPI *, __MaaGetController_t> {
    static constexpr const bool value = true;
};

static lhg::opaque_manager<MaaResourceAPI *> MaaResourceAPI__OpaqueManager;

template<>
struct lhg::schema_t<MaaResourceAPI *>
{
    static constexpr const char* const schema = "string@MaaResourceAPI";
};

template<>
struct lhg::is_opaque<MaaResourceAPI *> {
    static constexpr const bool value = true;
    using type = MaaResourceAPI;
    static constexpr const char* const name = "MaaResourceAPI";
    static lhg::opaque_manager<MaaResourceAPI *>& manager;
};
lhg::opaque_manager<MaaResourceAPI *>& lhg::is_opaque<MaaResourceAPI *>::manager = MaaResourceAPI__OpaqueManager;

template<>
struct lhg::is_opaque_free<MaaResourceAPI *, __MaaResourceDestroy_t> {
    static constexpr const bool value = true;
};

template<>
struct lhg::is_opaque_non_alloc<MaaResourceAPI *, __MaaGetResource_t> {
    static constexpr const bool value = true;
};

static lhg::opaque_manager<MaaInstanceAPI *> MaaInstanceAPI__OpaqueManager;

template<>
struct lhg::schema_t<MaaInstanceAPI *>
{
    static constexpr const char* const schema = "string@MaaInstanceAPI";
};

template<>
struct lhg::is_opaque<MaaInstanceAPI *> {
    static constexpr const bool value = true;
    using type = MaaInstanceAPI;
    static constexpr const char* const name = "MaaInstanceAPI";
    static lhg::opaque_manager<MaaInstanceAPI *>& manager;
};
lhg::opaque_manager<MaaInstanceAPI *>& lhg::is_opaque<MaaInstanceAPI *>::manager = MaaInstanceAPI__OpaqueManager;

template<>
struct lhg::is_opaque_free<MaaInstanceAPI *, __MaaDestroy_t> {
    static constexpr const bool value = true;
};

static lhg::opaque_manager<MaaImageBuffer *> MaaImageBuffer__OpaqueManager;

template<>
struct lhg::schema_t<MaaImageBuffer *>
{
    static constexpr const char* const schema = "string@MaaImageBuffer";
};

template<>
struct lhg::is_opaque<MaaImageBuffer *> {
    static constexpr const bool value = true;
    using type = MaaImageBuffer;
    static constexpr const char* const name = "MaaImageBuffer";
    static lhg::opaque_manager<MaaImageBuffer *>& manager;
};
lhg::opaque_manager<MaaImageBuffer *>& lhg::is_opaque<MaaImageBuffer *>::manager = MaaImageBuffer__OpaqueManager;

template<>
struct lhg::is_opaque_free<MaaImageBuffer *, __MaaDestroyImageBuffer_t> {
    static constexpr const bool value = true;
};

// LHG SEC BEGIN lhg.custom.MaaAdbControllerCreate.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaAdbControllerCreate_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaAdbControllerCreate_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaAdbControllerCreate_t>::temp_type __temp;
    typename lhg::arg_set<__MaaAdbControllerCreate_t>::call_type __call;
    if (!lhg::perform_input<__MaaAdbControllerCreate_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaAdbControllerCreate_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaAdbControllerCreate, __call);
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreate.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaAdbControllerCreate_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaWin32ControllerCreate.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaWin32ControllerCreate_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaWin32ControllerCreate_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaWin32ControllerCreate_t>::temp_type __temp;
    typename lhg::arg_set<__MaaWin32ControllerCreate_t>::call_type __call;
    if (!lhg::perform_input<__MaaWin32ControllerCreate_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaWin32ControllerCreate_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaWin32ControllerCreate, __call);
// LHG SEC BEGIN lhg.impl.MaaWin32ControllerCreate.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaWin32ControllerCreate_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaAdbControllerCreateV2.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaAdbControllerCreateV2_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaAdbControllerCreateV2_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaAdbControllerCreateV2_t>::temp_type __temp;
    typename lhg::arg_set<__MaaAdbControllerCreateV2_t>::call_type __call;
    if (!lhg::perform_input<__MaaAdbControllerCreateV2_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaAdbControllerCreateV2_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaAdbControllerCreateV2, __call);
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaAdbControllerCreateV2_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaThriftControllerCreate.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaThriftControllerCreate_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaThriftControllerCreate_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaThriftControllerCreate_t>::temp_type __temp;
    typename lhg::arg_set<__MaaThriftControllerCreate_t>::call_type __call;
    if (!lhg::perform_input<__MaaThriftControllerCreate_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaThriftControllerCreate_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaThriftControllerCreate, __call);
// LHG SEC BEGIN lhg.impl.MaaThriftControllerCreate.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaThriftControllerCreate_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaDbgControllerCreate.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaDbgControllerCreate_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaDbgControllerCreate_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaDbgControllerCreate_t>::temp_type __temp;
    typename lhg::arg_set<__MaaDbgControllerCreate_t>::call_type __call;
    if (!lhg::perform_input<__MaaDbgControllerCreate_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaDbgControllerCreate_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaDbgControllerCreate, __call);
// LHG SEC BEGIN lhg.impl.MaaDbgControllerCreate.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaDbgControllerCreate_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerDestroy.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerDestroy_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerDestroy_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerDestroy_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerDestroy_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerDestroy_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerDestroy_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    std::apply(MaaControllerDestroy, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerDestroy.return
// LHG SEC DEF
    int __ret = 0;
// LHG SEC END
    return lhg::perform_output<__MaaControllerDestroy_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostConnection.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostConnection_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerPostConnection_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerPostConnection_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerPostConnection_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerPostConnection_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerPostConnection_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostConnection, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostConnection.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerPostConnection_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostClick.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostClick_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerPostClick_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerPostClick_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerPostClick_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerPostClick_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerPostClick_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostClick, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostClick.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerPostClick_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostSwipe.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostSwipe_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerPostSwipe_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerPostSwipe_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerPostSwipe_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerPostSwipe_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerPostSwipe_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostSwipe, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostSwipe.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerPostSwipe_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostPressKey.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostPressKey_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerPostPressKey_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerPostPressKey_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerPostPressKey_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerPostPressKey_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerPostPressKey_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostPressKey, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostPressKey.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerPostPressKey_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostInputText.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostInputText_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerPostInputText_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerPostInputText_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerPostInputText_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerPostInputText_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerPostInputText_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostInputText, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostInputText.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerPostInputText_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostTouchDown.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostTouchDown_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerPostTouchDown_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerPostTouchDown_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerPostTouchDown_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerPostTouchDown_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerPostTouchDown_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostTouchDown, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchDown.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerPostTouchDown_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostTouchMove.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostTouchMove_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerPostTouchMove_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerPostTouchMove_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerPostTouchMove_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerPostTouchMove_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerPostTouchMove_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostTouchMove, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchMove.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerPostTouchMove_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostTouchUp.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostTouchUp_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerPostTouchUp_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerPostTouchUp_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerPostTouchUp_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerPostTouchUp_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerPostTouchUp_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostTouchUp, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchUp.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerPostTouchUp_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostScreencap.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostScreencap_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerPostScreencap_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerPostScreencap_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerPostScreencap_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerPostScreencap_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerPostScreencap_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostScreencap, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostScreencap.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerPostScreencap_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerStatus.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerStatus_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerStatus_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerStatus_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerStatus_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerStatus_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerStatus_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerStatus, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerStatus.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerStatus_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerWait.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerWait_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerWait_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerWait_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerWait_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerWait_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerWait_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerWait, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerWait.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerWait_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerConnected.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerConnected_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerConnected_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerConnected_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerConnected_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerConnected_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerConnected_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerConnected, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerConnected.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerConnected_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerGetImage.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerGetImage_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerGetImage_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerGetImage_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerGetImage_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerGetImage_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerGetImage_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerGetImage, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerGetImage.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerGetImage_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerGetUUID.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerGetUUID_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerGetUUID_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerGetUUID_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerGetUUID_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerGetUUID_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerGetUUID_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerGetUUID, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerGetUUID.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerGetUUID_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaCreate.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaCreate_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaCreate_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaCreate_t>::temp_type __temp;
    typename lhg::arg_set<__MaaCreate_t>::call_type __call;
    if (!lhg::perform_input<__MaaCreate_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaCreate_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaCreate, __call);
// LHG SEC BEGIN lhg.impl.MaaCreate.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaCreate_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaDestroy.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaDestroy_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaDestroy_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaDestroy_t>::temp_type __temp;
    typename lhg::arg_set<__MaaDestroy_t>::call_type __call;
    if (!lhg::perform_input<__MaaDestroy_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaDestroy_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    std::apply(MaaDestroy, __call);
// LHG SEC BEGIN lhg.impl.MaaDestroy.return
// LHG SEC DEF
    int __ret = 0;
// LHG SEC END
    return lhg::perform_output<__MaaDestroy_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaBindResource.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaBindResource_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaBindResource_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaBindResource_t>::temp_type __temp;
    typename lhg::arg_set<__MaaBindResource_t>::call_type __call;
    if (!lhg::perform_input<__MaaBindResource_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaBindResource_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaBindResource, __call);
// LHG SEC BEGIN lhg.impl.MaaBindResource.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaBindResource_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaBindController.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaBindController_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaBindController_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaBindController_t>::temp_type __temp;
    typename lhg::arg_set<__MaaBindController_t>::call_type __call;
    if (!lhg::perform_input<__MaaBindController_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaBindController_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaBindController, __call);
// LHG SEC BEGIN lhg.impl.MaaBindController.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaBindController_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaInited.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaInited_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaInited_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaInited_t>::temp_type __temp;
    typename lhg::arg_set<__MaaInited_t>::call_type __call;
    if (!lhg::perform_input<__MaaInited_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaInited_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaInited, __call);
// LHG SEC BEGIN lhg.impl.MaaInited.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaInited_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaClearCustomRecognizer.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaClearCustomRecognizer_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaClearCustomRecognizer_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaClearCustomRecognizer_t>::temp_type __temp;
    typename lhg::arg_set<__MaaClearCustomRecognizer_t>::call_type __call;
    if (!lhg::perform_input<__MaaClearCustomRecognizer_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaClearCustomRecognizer_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaClearCustomRecognizer, __call);
// LHG SEC BEGIN lhg.impl.MaaClearCustomRecognizer.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaClearCustomRecognizer_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaClearCustomAction.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaClearCustomAction_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaClearCustomAction_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaClearCustomAction_t>::temp_type __temp;
    typename lhg::arg_set<__MaaClearCustomAction_t>::call_type __call;
    if (!lhg::perform_input<__MaaClearCustomAction_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaClearCustomAction_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaClearCustomAction, __call);
// LHG SEC BEGIN lhg.impl.MaaClearCustomAction.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaClearCustomAction_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaPostTask.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaPostTask_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaPostTask_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaPostTask_t>::temp_type __temp;
    typename lhg::arg_set<__MaaPostTask_t>::call_type __call;
    if (!lhg::perform_input<__MaaPostTask_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaPostTask_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaPostTask, __call);
// LHG SEC BEGIN lhg.impl.MaaPostTask.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaPostTask_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaSetTaskParam.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaSetTaskParam_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaSetTaskParam_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaSetTaskParam_t>::temp_type __temp;
    typename lhg::arg_set<__MaaSetTaskParam_t>::call_type __call;
    if (!lhg::perform_input<__MaaSetTaskParam_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaSetTaskParam_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaSetTaskParam, __call);
// LHG SEC BEGIN lhg.impl.MaaSetTaskParam.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaSetTaskParam_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaTaskStatus.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaTaskStatus_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaTaskStatus_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaTaskStatus_t>::temp_type __temp;
    typename lhg::arg_set<__MaaTaskStatus_t>::call_type __call;
    if (!lhg::perform_input<__MaaTaskStatus_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaTaskStatus_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaTaskStatus, __call);
// LHG SEC BEGIN lhg.impl.MaaTaskStatus.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaTaskStatus_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaWaitTask.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaWaitTask_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaWaitTask_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaWaitTask_t>::temp_type __temp;
    typename lhg::arg_set<__MaaWaitTask_t>::call_type __call;
    if (!lhg::perform_input<__MaaWaitTask_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaWaitTask_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaWaitTask, __call);
// LHG SEC BEGIN lhg.impl.MaaWaitTask.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaWaitTask_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaTaskAllFinished.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaTaskAllFinished_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaTaskAllFinished_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaTaskAllFinished_t>::temp_type __temp;
    typename lhg::arg_set<__MaaTaskAllFinished_t>::call_type __call;
    if (!lhg::perform_input<__MaaTaskAllFinished_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaTaskAllFinished_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaTaskAllFinished, __call);
// LHG SEC BEGIN lhg.impl.MaaTaskAllFinished.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaTaskAllFinished_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaPostStop.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaPostStop_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaPostStop_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaPostStop_t>::temp_type __temp;
    typename lhg::arg_set<__MaaPostStop_t>::call_type __call;
    if (!lhg::perform_input<__MaaPostStop_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaPostStop_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaPostStop, __call);
// LHG SEC BEGIN lhg.impl.MaaPostStop.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaPostStop_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaStop.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaStop_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaStop_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaStop_t>::temp_type __temp;
    typename lhg::arg_set<__MaaStop_t>::call_type __call;
    if (!lhg::perform_input<__MaaStop_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaStop_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaStop, __call);
// LHG SEC BEGIN lhg.impl.MaaStop.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaStop_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaGetResource.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaGetResource_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaGetResource_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaGetResource_t>::temp_type __temp;
    typename lhg::arg_set<__MaaGetResource_t>::call_type __call;
    if (!lhg::perform_input<__MaaGetResource_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaGetResource_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaGetResource, __call);
// LHG SEC BEGIN lhg.impl.MaaGetResource.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaGetResource_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaGetController.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaGetController_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaGetController_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaGetController_t>::temp_type __temp;
    typename lhg::arg_set<__MaaGetController_t>::call_type __call;
    if (!lhg::perform_input<__MaaGetController_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaGetController_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaGetController, __call);
// LHG SEC BEGIN lhg.impl.MaaGetController.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaGetController_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaResourceCreate.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaResourceCreate_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaResourceCreate_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaResourceCreate_t>::temp_type __temp;
    typename lhg::arg_set<__MaaResourceCreate_t>::call_type __call;
    if (!lhg::perform_input<__MaaResourceCreate_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaResourceCreate_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaResourceCreate, __call);
// LHG SEC BEGIN lhg.impl.MaaResourceCreate.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaResourceCreate_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaResourceDestroy.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaResourceDestroy_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaResourceDestroy_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaResourceDestroy_t>::temp_type __temp;
    typename lhg::arg_set<__MaaResourceDestroy_t>::call_type __call;
    if (!lhg::perform_input<__MaaResourceDestroy_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaResourceDestroy_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    std::apply(MaaResourceDestroy, __call);
// LHG SEC BEGIN lhg.impl.MaaResourceDestroy.return
// LHG SEC DEF
    int __ret = 0;
// LHG SEC END
    return lhg::perform_output<__MaaResourceDestroy_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaResourcePostPath.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaResourcePostPath_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaResourcePostPath_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaResourcePostPath_t>::temp_type __temp;
    typename lhg::arg_set<__MaaResourcePostPath_t>::call_type __call;
    if (!lhg::perform_input<__MaaResourcePostPath_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaResourcePostPath_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaResourcePostPath, __call);
// LHG SEC BEGIN lhg.impl.MaaResourcePostPath.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaResourcePostPath_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaResourceStatus.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaResourceStatus_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaResourceStatus_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaResourceStatus_t>::temp_type __temp;
    typename lhg::arg_set<__MaaResourceStatus_t>::call_type __call;
    if (!lhg::perform_input<__MaaResourceStatus_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaResourceStatus_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaResourceStatus, __call);
// LHG SEC BEGIN lhg.impl.MaaResourceStatus.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaResourceStatus_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaResourceWait.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaResourceWait_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaResourceWait_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaResourceWait_t>::temp_type __temp;
    typename lhg::arg_set<__MaaResourceWait_t>::call_type __call;
    if (!lhg::perform_input<__MaaResourceWait_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaResourceWait_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaResourceWait, __call);
// LHG SEC BEGIN lhg.impl.MaaResourceWait.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaResourceWait_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaResourceLoaded.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaResourceLoaded_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaResourceLoaded_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaResourceLoaded_t>::temp_type __temp;
    typename lhg::arg_set<__MaaResourceLoaded_t>::call_type __call;
    if (!lhg::perform_input<__MaaResourceLoaded_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaResourceLoaded_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaResourceLoaded, __call);
// LHG SEC BEGIN lhg.impl.MaaResourceLoaded.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaResourceLoaded_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaResourceGetHash.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaResourceGetHash_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaResourceGetHash_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaResourceGetHash_t>::temp_type __temp;
    typename lhg::arg_set<__MaaResourceGetHash_t>::call_type __call;
    if (!lhg::perform_input<__MaaResourceGetHash_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaResourceGetHash_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaResourceGetHash, __call);
// LHG SEC BEGIN lhg.impl.MaaResourceGetHash.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaResourceGetHash_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaResourceGetTaskList.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaResourceGetTaskList_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaResourceGetTaskList_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaResourceGetTaskList_t>::temp_type __temp;
    typename lhg::arg_set<__MaaResourceGetTaskList_t>::call_type __call;
    if (!lhg::perform_input<__MaaResourceGetTaskList_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaResourceGetTaskList_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaResourceGetTaskList, __call);
// LHG SEC BEGIN lhg.impl.MaaResourceGetTaskList.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaResourceGetTaskList_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaCreateImageBuffer.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaCreateImageBuffer_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaCreateImageBuffer_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaCreateImageBuffer_t>::temp_type __temp;
    typename lhg::arg_set<__MaaCreateImageBuffer_t>::call_type __call;
    if (!lhg::perform_input<__MaaCreateImageBuffer_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaCreateImageBuffer_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaCreateImageBuffer();
// LHG SEC BEGIN lhg.impl.MaaCreateImageBuffer.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaCreateImageBuffer_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaDestroyImageBuffer.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaDestroyImageBuffer_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaDestroyImageBuffer_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaDestroyImageBuffer_t>::temp_type __temp;
    typename lhg::arg_set<__MaaDestroyImageBuffer_t>::call_type __call;
    if (!lhg::perform_input<__MaaDestroyImageBuffer_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaDestroyImageBuffer_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    std::apply(MaaDestroyImageBuffer, __call);
// LHG SEC BEGIN lhg.impl.MaaDestroyImageBuffer.return
// LHG SEC DEF
    int __ret = 0;
// LHG SEC END
    return lhg::perform_output<__MaaDestroyImageBuffer_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaIsImageEmpty.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaIsImageEmpty_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaIsImageEmpty_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaIsImageEmpty_t>::temp_type __temp;
    typename lhg::arg_set<__MaaIsImageEmpty_t>::call_type __call;
    if (!lhg::perform_input<__MaaIsImageEmpty_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaIsImageEmpty_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaIsImageEmpty, __call);
// LHG SEC BEGIN lhg.impl.MaaIsImageEmpty.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaIsImageEmpty_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaClearImage.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaClearImage_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaClearImage_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaClearImage_t>::temp_type __temp;
    typename lhg::arg_set<__MaaClearImage_t>::call_type __call;
    if (!lhg::perform_input<__MaaClearImage_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaClearImage_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaClearImage, __call);
// LHG SEC BEGIN lhg.impl.MaaClearImage.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaClearImage_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaGetImageWidth.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaGetImageWidth_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaGetImageWidth_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaGetImageWidth_t>::temp_type __temp;
    typename lhg::arg_set<__MaaGetImageWidth_t>::call_type __call;
    if (!lhg::perform_input<__MaaGetImageWidth_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaGetImageWidth_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaGetImageWidth, __call);
// LHG SEC BEGIN lhg.impl.MaaGetImageWidth.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaGetImageWidth_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaGetImageHeight.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaGetImageHeight_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaGetImageHeight_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaGetImageHeight_t>::temp_type __temp;
    typename lhg::arg_set<__MaaGetImageHeight_t>::call_type __call;
    if (!lhg::perform_input<__MaaGetImageHeight_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaGetImageHeight_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaGetImageHeight, __call);
// LHG SEC BEGIN lhg.impl.MaaGetImageHeight.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaGetImageHeight_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaGetImageType.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaGetImageType_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaGetImageType_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaGetImageType_t>::temp_type __temp;
    typename lhg::arg_set<__MaaGetImageType_t>::call_type __call;
    if (!lhg::perform_input<__MaaGetImageType_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaGetImageType_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaGetImageType, __call);
// LHG SEC BEGIN lhg.impl.MaaGetImageType.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaGetImageType_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaGetImageEncoded.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaGetImageEncoded_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaGetImageEncoded_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaGetImageEncoded_t>::temp_type __temp;
    typename lhg::arg_set<__MaaGetImageEncoded_t>::call_type __call;
    if (!lhg::perform_input<__MaaGetImageEncoded_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaGetImageEncoded_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaGetImageEncoded, __call);
// LHG SEC BEGIN lhg.impl.MaaGetImageEncoded.return
    std::string __ret(reinterpret_cast<char*>(__return), MaaGetImageEncodedSize(std::get<0>(__call)));
// LHG SEC END
    return lhg::perform_output<__MaaGetImageEncoded_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaSetImageEncoded.func
template<>
inline bool lhg::perform_check_entry<__MaaSetImageEncoded_t::__data_t>(json::object& __param, std::string& __error) {
    if (!lhg::check_var<const char *>(__param["data"])) {
        __error = "data should be string@buffer";
        return false;
    }
    return true;
}

template<>
inline bool lhg::perform_check_entry<__MaaSetImageEncoded_t::__size_t>(json::object& __param, std::string& __error) {
    return true;
}

template<>
struct lhg::temp_arg_t<__MaaSetImageEncoded_t::__data_t> {
    using type = std::string;
};

template<>
struct lhg::temp_arg_t<__MaaSetImageEncoded_t::__size_t> {
    using type = std::monostate;
};

template<>
inline bool lhg::perform_input_entry<__MaaSetImageEncoded_t, __MaaSetImageEncoded_t::__data_t>(typename lhg::arg_set<__MaaSetImageEncoded_t>::temp_type& temp_data, json::object __param, std::string& __error) {
    std::get<__MaaSetImageEncoded_t::__data_t::index>(temp_data) = __param[__MaaSetImageEncoded_t::__data_t::name].as_string();
    return true;
}

template<>
inline bool lhg::perform_input_entry<__MaaSetImageEncoded_t, __MaaSetImageEncoded_t::__size_t>(typename lhg::arg_set<__MaaSetImageEncoded_t>::temp_type& temp_data, json::object __param, std::string& __error) {
    return true;
}

template<>
inline bool lhg::perform_input_fix_entry<__MaaSetImageEncoded_t, __MaaSetImageEncoded_t::__data_t>(typename lhg::arg_set<__MaaSetImageEncoded_t>::call_type& data, typename lhg::arg_set<__MaaSetImageEncoded_t>::temp_type& temp_data, json::object __param, std::string& __error) {
    const auto &str = std::get<__MaaSetImageEncoded_t::__data_t::index>(temp_data);
    std::get<__MaaSetImageEncoded_t::__data_t::index>(data) = reinterpret_cast<uint8_t *>(const_cast<char *>(str.c_str()));
    return true;
}

template<>
inline bool lhg::perform_input_fix_entry<__MaaSetImageEncoded_t, __MaaSetImageEncoded_t::__size_t>(typename lhg::arg_set<__MaaSetImageEncoded_t>::call_type& data, typename lhg::arg_set<__MaaSetImageEncoded_t>::temp_type& temp_data, json::object __param, std::string& __error) {
    const auto &str = std::get<__MaaSetImageEncoded_t::__data_t::index>(temp_data);
    std::get<__MaaSetImageEncoded_t::__size_t::index>(data) = str.size();
    return true;
}
// LHG SEC END
std::optional<json::object> MaaSetImageEncoded_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaSetImageEncoded_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaSetImageEncoded_t>::temp_type __temp;
    typename lhg::arg_set<__MaaSetImageEncoded_t>::call_type __call;
    if (!lhg::perform_input<__MaaSetImageEncoded_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaSetImageEncoded_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaSetImageEncoded, __call);
// LHG SEC BEGIN lhg.impl.MaaSetImageEncoded.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaSetImageEncoded_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaVersion.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaVersion_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaVersion_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaVersion_t>::temp_type __temp;
    typename lhg::arg_set<__MaaVersion_t>::call_type __call;
    if (!lhg::perform_input<__MaaVersion_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaVersion_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaVersion();
// LHG SEC BEGIN lhg.impl.MaaVersion.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaVersion_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitInit.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitInit_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitInit_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitInit_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitInit_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitInit_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitInit_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitInit();
// LHG SEC BEGIN lhg.impl.MaaToolkitInit.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitInit_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitUninit.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitUninit_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitUninit_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitUninit_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitUninit_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitUninit_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitUninit_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitUninit();
// LHG SEC BEGIN lhg.impl.MaaToolkitUninit.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitUninit_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitFindDevice.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitFindDevice_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitFindDevice_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitFindDevice_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitFindDevice_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitFindDevice_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitFindDevice_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitFindDevice();
// LHG SEC BEGIN lhg.impl.MaaToolkitFindDevice.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitFindDevice_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitFindDeviceWithAdb.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitFindDeviceWithAdb_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitFindDeviceWithAdb_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitFindDeviceWithAdb_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitFindDeviceWithAdb_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitFindDeviceWithAdb_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitFindDeviceWithAdb_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitFindDeviceWithAdb, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitFindDeviceWithAdb.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitFindDeviceWithAdb_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitPostFindDevice.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitPostFindDevice_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitPostFindDevice_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitPostFindDevice_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitPostFindDevice_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitPostFindDevice_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitPostFindDevice_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitPostFindDevice();
// LHG SEC BEGIN lhg.impl.MaaToolkitPostFindDevice.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitPostFindDevice_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitPostFindDeviceWithAdb.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitPostFindDeviceWithAdb_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitPostFindDeviceWithAdb_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitPostFindDeviceWithAdb_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitPostFindDeviceWithAdb_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitPostFindDeviceWithAdb_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitPostFindDeviceWithAdb_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitPostFindDeviceWithAdb, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitPostFindDeviceWithAdb.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitPostFindDeviceWithAdb_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitIsFindDeviceCompleted.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitIsFindDeviceCompleted_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitIsFindDeviceCompleted_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitIsFindDeviceCompleted_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitIsFindDeviceCompleted_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitIsFindDeviceCompleted_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitIsFindDeviceCompleted_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitIsFindDeviceCompleted();
// LHG SEC BEGIN lhg.impl.MaaToolkitIsFindDeviceCompleted.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitIsFindDeviceCompleted_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitWaitForFindDeviceToComplete.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitWaitForFindDeviceToComplete_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitWaitForFindDeviceToComplete_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitWaitForFindDeviceToComplete_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitWaitForFindDeviceToComplete_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitWaitForFindDeviceToComplete_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitWaitForFindDeviceToComplete_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitWaitForFindDeviceToComplete();
// LHG SEC BEGIN lhg.impl.MaaToolkitWaitForFindDeviceToComplete.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitWaitForFindDeviceToComplete_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceCount.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetDeviceCount_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitGetDeviceCount_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitGetDeviceCount_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitGetDeviceCount_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitGetDeviceCount_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitGetDeviceCount_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitGetDeviceCount();
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceCount.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitGetDeviceCount_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceName.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetDeviceName_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitGetDeviceName_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitGetDeviceName_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitGetDeviceName_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitGetDeviceName_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitGetDeviceName_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitGetDeviceName, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceName.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitGetDeviceName_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceAdbPath.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetDeviceAdbPath_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitGetDeviceAdbPath_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitGetDeviceAdbPath_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitGetDeviceAdbPath_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitGetDeviceAdbPath_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitGetDeviceAdbPath_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitGetDeviceAdbPath, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbPath.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitGetDeviceAdbPath_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceAdbSerial.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetDeviceAdbSerial_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitGetDeviceAdbSerial_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitGetDeviceAdbSerial_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitGetDeviceAdbSerial_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitGetDeviceAdbSerial_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitGetDeviceAdbSerial_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitGetDeviceAdbSerial, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbSerial.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitGetDeviceAdbSerial_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceAdbControllerType.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetDeviceAdbControllerType_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitGetDeviceAdbControllerType_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitGetDeviceAdbControllerType_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitGetDeviceAdbControllerType_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitGetDeviceAdbControllerType_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitGetDeviceAdbControllerType_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitGetDeviceAdbControllerType, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbControllerType.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitGetDeviceAdbControllerType_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceAdbConfig.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetDeviceAdbConfig_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitGetDeviceAdbConfig_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitGetDeviceAdbConfig_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitGetDeviceAdbConfig_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitGetDeviceAdbConfig_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitGetDeviceAdbConfig_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitGetDeviceAdbConfig, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbConfig.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitGetDeviceAdbConfig_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitRegisterCustomRecognizerExecutor.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitRegisterCustomRecognizerExecutor_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitRegisterCustomRecognizerExecutor_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitRegisterCustomRecognizerExecutor_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitRegisterCustomRecognizerExecutor_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitRegisterCustomRecognizerExecutor_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitRegisterCustomRecognizerExecutor_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitRegisterCustomRecognizerExecutor, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomRecognizerExecutor.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitRegisterCustomRecognizerExecutor_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitUnregisterCustomRecognizerExecutor.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitUnregisterCustomRecognizerExecutor_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitUnregisterCustomRecognizerExecutor_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitUnregisterCustomRecognizerExecutor_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitUnregisterCustomRecognizerExecutor_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitUnregisterCustomRecognizerExecutor_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitUnregisterCustomRecognizerExecutor_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitUnregisterCustomRecognizerExecutor, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitUnregisterCustomRecognizerExecutor.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitUnregisterCustomRecognizerExecutor_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitRegisterCustomActionExecutor.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitRegisterCustomActionExecutor_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitRegisterCustomActionExecutor_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitRegisterCustomActionExecutor_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitRegisterCustomActionExecutor_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitRegisterCustomActionExecutor_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitRegisterCustomActionExecutor_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitRegisterCustomActionExecutor, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomActionExecutor.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitRegisterCustomActionExecutor_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitUnregisterCustomActionExecutor.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitUnregisterCustomActionExecutor_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitUnregisterCustomActionExecutor_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitUnregisterCustomActionExecutor_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitUnregisterCustomActionExecutor_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitUnregisterCustomActionExecutor_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitUnregisterCustomActionExecutor_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitUnregisterCustomActionExecutor, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitUnregisterCustomActionExecutor.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitUnregisterCustomActionExecutor_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitFindWindow.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitFindWindow_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitFindWindow_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitFindWindow_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitFindWindow_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitFindWindow_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitFindWindow_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitFindWindow, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitFindWindow.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitFindWindow_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitSearchWindow.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitSearchWindow_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitSearchWindow_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitSearchWindow_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitSearchWindow_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitSearchWindow_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitSearchWindow_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitSearchWindow, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitSearchWindow.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitSearchWindow_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetWindow.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetWindow_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitGetWindow_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitGetWindow_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitGetWindow_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitGetWindow_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitGetWindow_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitGetWindow, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitGetWindow.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitGetWindow_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetCursorWindow.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetCursorWindow_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaToolkitGetCursorWindow_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaToolkitGetCursorWindow_t>::temp_type __temp;
    typename lhg::arg_set<__MaaToolkitGetCursorWindow_t>::call_type __call;
    if (!lhg::perform_input<__MaaToolkitGetCursorWindow_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaToolkitGetCursorWindow_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitGetCursorWindow();
// LHG SEC BEGIN lhg.impl.MaaToolkitGetCursorWindow.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaToolkitGetCursorWindow_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerSetOptionString.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerSetOptionString_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerSetOptionString_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerSetOptionString_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerSetOptionString_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerSetOptionString_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerSetOptionString_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerSetOptionString, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerSetOptionString.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerSetOptionString_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerSetOptionInteger.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerSetOptionInteger_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerSetOptionInteger_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerSetOptionInteger_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerSetOptionInteger_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerSetOptionInteger_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerSetOptionInteger_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerSetOptionInteger, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerSetOptionInteger.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerSetOptionInteger_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerSetOptionBoolean.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerSetOptionBoolean_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaControllerSetOptionBoolean_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaControllerSetOptionBoolean_t>::temp_type __temp;
    typename lhg::arg_set<__MaaControllerSetOptionBoolean_t>::call_type __call;
    if (!lhg::perform_input<__MaaControllerSetOptionBoolean_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaControllerSetOptionBoolean_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerSetOptionBoolean, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerSetOptionBoolean.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaControllerSetOptionBoolean_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaSetGlobalOptionString.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaSetGlobalOptionString_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaSetGlobalOptionString_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaSetGlobalOptionString_t>::temp_type __temp;
    typename lhg::arg_set<__MaaSetGlobalOptionString_t>::call_type __call;
    if (!lhg::perform_input<__MaaSetGlobalOptionString_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaSetGlobalOptionString_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaSetGlobalOptionString, __call);
// LHG SEC BEGIN lhg.impl.MaaSetGlobalOptionString.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaSetGlobalOptionString_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaSetGlobalOptionInteger.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaSetGlobalOptionInteger_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaSetGlobalOptionInteger_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaSetGlobalOptionInteger_t>::temp_type __temp;
    typename lhg::arg_set<__MaaSetGlobalOptionInteger_t>::call_type __call;
    if (!lhg::perform_input<__MaaSetGlobalOptionInteger_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaSetGlobalOptionInteger_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaSetGlobalOptionInteger, __call);
// LHG SEC BEGIN lhg.impl.MaaSetGlobalOptionInteger.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaSetGlobalOptionInteger_t>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaSetGlobalOptionBoolean.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaSetGlobalOptionBoolean_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<__MaaSetGlobalOptionBoolean_t>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<__MaaSetGlobalOptionBoolean_t>::temp_type __temp;
    typename lhg::arg_set<__MaaSetGlobalOptionBoolean_t>::call_type __call;
    if (!lhg::perform_input<__MaaSetGlobalOptionBoolean_t>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<__MaaSetGlobalOptionBoolean_t>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaSetGlobalOptionBoolean, __call);
// LHG SEC BEGIN lhg.impl.MaaSetGlobalOptionBoolean.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<__MaaSetGlobalOptionBoolean_t>(__call, __ret);
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
        { "MaaAdbControllerCreate", { &MaaAdbControllerCreate_Wrapper, &lhg::input_helper<__MaaAdbControllerCreate_t>, &lhg::output_helper<__MaaAdbControllerCreate_t> } },
        { "MaaWin32ControllerCreate", { &MaaWin32ControllerCreate_Wrapper, &lhg::input_helper<__MaaWin32ControllerCreate_t>, &lhg::output_helper<__MaaWin32ControllerCreate_t> } },
        { "MaaAdbControllerCreateV2", { &MaaAdbControllerCreateV2_Wrapper, &lhg::input_helper<__MaaAdbControllerCreateV2_t>, &lhg::output_helper<__MaaAdbControllerCreateV2_t> } },
        { "MaaThriftControllerCreate", { &MaaThriftControllerCreate_Wrapper, &lhg::input_helper<__MaaThriftControllerCreate_t>, &lhg::output_helper<__MaaThriftControllerCreate_t> } },
        { "MaaDbgControllerCreate", { &MaaDbgControllerCreate_Wrapper, &lhg::input_helper<__MaaDbgControllerCreate_t>, &lhg::output_helper<__MaaDbgControllerCreate_t> } },
        { "MaaControllerDestroy", { &MaaControllerDestroy_Wrapper, &lhg::input_helper<__MaaControllerDestroy_t>, &lhg::output_helper<__MaaControllerDestroy_t> } },
        { "MaaControllerPostConnection", { &MaaControllerPostConnection_Wrapper, &lhg::input_helper<__MaaControllerPostConnection_t>, &lhg::output_helper<__MaaControllerPostConnection_t> } },
        { "MaaControllerPostClick", { &MaaControllerPostClick_Wrapper, &lhg::input_helper<__MaaControllerPostClick_t>, &lhg::output_helper<__MaaControllerPostClick_t> } },
        { "MaaControllerPostSwipe", { &MaaControllerPostSwipe_Wrapper, &lhg::input_helper<__MaaControllerPostSwipe_t>, &lhg::output_helper<__MaaControllerPostSwipe_t> } },
        { "MaaControllerPostPressKey", { &MaaControllerPostPressKey_Wrapper, &lhg::input_helper<__MaaControllerPostPressKey_t>, &lhg::output_helper<__MaaControllerPostPressKey_t> } },
        { "MaaControllerPostInputText", { &MaaControllerPostInputText_Wrapper, &lhg::input_helper<__MaaControllerPostInputText_t>, &lhg::output_helper<__MaaControllerPostInputText_t> } },
        { "MaaControllerPostTouchDown", { &MaaControllerPostTouchDown_Wrapper, &lhg::input_helper<__MaaControllerPostTouchDown_t>, &lhg::output_helper<__MaaControllerPostTouchDown_t> } },
        { "MaaControllerPostTouchMove", { &MaaControllerPostTouchMove_Wrapper, &lhg::input_helper<__MaaControllerPostTouchMove_t>, &lhg::output_helper<__MaaControllerPostTouchMove_t> } },
        { "MaaControllerPostTouchUp", { &MaaControllerPostTouchUp_Wrapper, &lhg::input_helper<__MaaControllerPostTouchUp_t>, &lhg::output_helper<__MaaControllerPostTouchUp_t> } },
        { "MaaControllerPostScreencap", { &MaaControllerPostScreencap_Wrapper, &lhg::input_helper<__MaaControllerPostScreencap_t>, &lhg::output_helper<__MaaControllerPostScreencap_t> } },
        { "MaaControllerStatus", { &MaaControllerStatus_Wrapper, &lhg::input_helper<__MaaControllerStatus_t>, &lhg::output_helper<__MaaControllerStatus_t> } },
        { "MaaControllerWait", { &MaaControllerWait_Wrapper, &lhg::input_helper<__MaaControllerWait_t>, &lhg::output_helper<__MaaControllerWait_t> } },
        { "MaaControllerConnected", { &MaaControllerConnected_Wrapper, &lhg::input_helper<__MaaControllerConnected_t>, &lhg::output_helper<__MaaControllerConnected_t> } },
        { "MaaControllerGetImage", { &MaaControllerGetImage_Wrapper, &lhg::input_helper<__MaaControllerGetImage_t>, &lhg::output_helper<__MaaControllerGetImage_t> } },
        { "MaaControllerGetUUID", { &MaaControllerGetUUID_Wrapper, &lhg::input_helper<__MaaControllerGetUUID_t>, &lhg::output_helper<__MaaControllerGetUUID_t> } },
        { "MaaCreate", { &MaaCreate_Wrapper, &lhg::input_helper<__MaaCreate_t>, &lhg::output_helper<__MaaCreate_t> } },
        { "MaaDestroy", { &MaaDestroy_Wrapper, &lhg::input_helper<__MaaDestroy_t>, &lhg::output_helper<__MaaDestroy_t> } },
        { "MaaBindResource", { &MaaBindResource_Wrapper, &lhg::input_helper<__MaaBindResource_t>, &lhg::output_helper<__MaaBindResource_t> } },
        { "MaaBindController", { &MaaBindController_Wrapper, &lhg::input_helper<__MaaBindController_t>, &lhg::output_helper<__MaaBindController_t> } },
        { "MaaInited", { &MaaInited_Wrapper, &lhg::input_helper<__MaaInited_t>, &lhg::output_helper<__MaaInited_t> } },
        { "MaaClearCustomRecognizer", { &MaaClearCustomRecognizer_Wrapper, &lhg::input_helper<__MaaClearCustomRecognizer_t>, &lhg::output_helper<__MaaClearCustomRecognizer_t> } },
        { "MaaClearCustomAction", { &MaaClearCustomAction_Wrapper, &lhg::input_helper<__MaaClearCustomAction_t>, &lhg::output_helper<__MaaClearCustomAction_t> } },
        { "MaaPostTask", { &MaaPostTask_Wrapper, &lhg::input_helper<__MaaPostTask_t>, &lhg::output_helper<__MaaPostTask_t> } },
        { "MaaSetTaskParam", { &MaaSetTaskParam_Wrapper, &lhg::input_helper<__MaaSetTaskParam_t>, &lhg::output_helper<__MaaSetTaskParam_t> } },
        { "MaaTaskStatus", { &MaaTaskStatus_Wrapper, &lhg::input_helper<__MaaTaskStatus_t>, &lhg::output_helper<__MaaTaskStatus_t> } },
        { "MaaWaitTask", { &MaaWaitTask_Wrapper, &lhg::input_helper<__MaaWaitTask_t>, &lhg::output_helper<__MaaWaitTask_t> } },
        { "MaaTaskAllFinished", { &MaaTaskAllFinished_Wrapper, &lhg::input_helper<__MaaTaskAllFinished_t>, &lhg::output_helper<__MaaTaskAllFinished_t> } },
        { "MaaPostStop", { &MaaPostStop_Wrapper, &lhg::input_helper<__MaaPostStop_t>, &lhg::output_helper<__MaaPostStop_t> } },
        { "MaaStop", { &MaaStop_Wrapper, &lhg::input_helper<__MaaStop_t>, &lhg::output_helper<__MaaStop_t> } },
        { "MaaGetResource", { &MaaGetResource_Wrapper, &lhg::input_helper<__MaaGetResource_t>, &lhg::output_helper<__MaaGetResource_t> } },
        { "MaaGetController", { &MaaGetController_Wrapper, &lhg::input_helper<__MaaGetController_t>, &lhg::output_helper<__MaaGetController_t> } },
        { "MaaResourceCreate", { &MaaResourceCreate_Wrapper, &lhg::input_helper<__MaaResourceCreate_t>, &lhg::output_helper<__MaaResourceCreate_t> } },
        { "MaaResourceDestroy", { &MaaResourceDestroy_Wrapper, &lhg::input_helper<__MaaResourceDestroy_t>, &lhg::output_helper<__MaaResourceDestroy_t> } },
        { "MaaResourcePostPath", { &MaaResourcePostPath_Wrapper, &lhg::input_helper<__MaaResourcePostPath_t>, &lhg::output_helper<__MaaResourcePostPath_t> } },
        { "MaaResourceStatus", { &MaaResourceStatus_Wrapper, &lhg::input_helper<__MaaResourceStatus_t>, &lhg::output_helper<__MaaResourceStatus_t> } },
        { "MaaResourceWait", { &MaaResourceWait_Wrapper, &lhg::input_helper<__MaaResourceWait_t>, &lhg::output_helper<__MaaResourceWait_t> } },
        { "MaaResourceLoaded", { &MaaResourceLoaded_Wrapper, &lhg::input_helper<__MaaResourceLoaded_t>, &lhg::output_helper<__MaaResourceLoaded_t> } },
        { "MaaResourceGetHash", { &MaaResourceGetHash_Wrapper, &lhg::input_helper<__MaaResourceGetHash_t>, &lhg::output_helper<__MaaResourceGetHash_t> } },
        { "MaaResourceGetTaskList", { &MaaResourceGetTaskList_Wrapper, &lhg::input_helper<__MaaResourceGetTaskList_t>, &lhg::output_helper<__MaaResourceGetTaskList_t> } },
        { "MaaCreateImageBuffer", { &MaaCreateImageBuffer_Wrapper, &lhg::input_helper<__MaaCreateImageBuffer_t>, &lhg::output_helper<__MaaCreateImageBuffer_t> } },
        { "MaaDestroyImageBuffer", { &MaaDestroyImageBuffer_Wrapper, &lhg::input_helper<__MaaDestroyImageBuffer_t>, &lhg::output_helper<__MaaDestroyImageBuffer_t> } },
        { "MaaIsImageEmpty", { &MaaIsImageEmpty_Wrapper, &lhg::input_helper<__MaaIsImageEmpty_t>, &lhg::output_helper<__MaaIsImageEmpty_t> } },
        { "MaaClearImage", { &MaaClearImage_Wrapper, &lhg::input_helper<__MaaClearImage_t>, &lhg::output_helper<__MaaClearImage_t> } },
        { "MaaGetImageWidth", { &MaaGetImageWidth_Wrapper, &lhg::input_helper<__MaaGetImageWidth_t>, &lhg::output_helper<__MaaGetImageWidth_t> } },
        { "MaaGetImageHeight", { &MaaGetImageHeight_Wrapper, &lhg::input_helper<__MaaGetImageHeight_t>, &lhg::output_helper<__MaaGetImageHeight_t> } },
        { "MaaGetImageType", { &MaaGetImageType_Wrapper, &lhg::input_helper<__MaaGetImageType_t>, &lhg::output_helper<__MaaGetImageType_t> } },
        { "MaaGetImageEncoded", { &MaaGetImageEncoded_Wrapper, &lhg::input_helper<__MaaGetImageEncoded_t>, &lhg::output_helper<__MaaGetImageEncoded_t> } },
        { "MaaSetImageEncoded", { &MaaSetImageEncoded_Wrapper, &lhg::input_helper<__MaaSetImageEncoded_t>, &lhg::output_helper<__MaaSetImageEncoded_t> } },
        { "MaaVersion", { &MaaVersion_Wrapper, &lhg::input_helper<__MaaVersion_t>, &lhg::output_helper<__MaaVersion_t> } },
        { "MaaToolkitInit", { &MaaToolkitInit_Wrapper, &lhg::input_helper<__MaaToolkitInit_t>, &lhg::output_helper<__MaaToolkitInit_t> } },
        { "MaaToolkitUninit", { &MaaToolkitUninit_Wrapper, &lhg::input_helper<__MaaToolkitUninit_t>, &lhg::output_helper<__MaaToolkitUninit_t> } },
        { "MaaToolkitFindDevice", { &MaaToolkitFindDevice_Wrapper, &lhg::input_helper<__MaaToolkitFindDevice_t>, &lhg::output_helper<__MaaToolkitFindDevice_t> } },
        { "MaaToolkitFindDeviceWithAdb", { &MaaToolkitFindDeviceWithAdb_Wrapper, &lhg::input_helper<__MaaToolkitFindDeviceWithAdb_t>, &lhg::output_helper<__MaaToolkitFindDeviceWithAdb_t> } },
        { "MaaToolkitPostFindDevice", { &MaaToolkitPostFindDevice_Wrapper, &lhg::input_helper<__MaaToolkitPostFindDevice_t>, &lhg::output_helper<__MaaToolkitPostFindDevice_t> } },
        { "MaaToolkitPostFindDeviceWithAdb", { &MaaToolkitPostFindDeviceWithAdb_Wrapper, &lhg::input_helper<__MaaToolkitPostFindDeviceWithAdb_t>, &lhg::output_helper<__MaaToolkitPostFindDeviceWithAdb_t> } },
        { "MaaToolkitIsFindDeviceCompleted", { &MaaToolkitIsFindDeviceCompleted_Wrapper, &lhg::input_helper<__MaaToolkitIsFindDeviceCompleted_t>, &lhg::output_helper<__MaaToolkitIsFindDeviceCompleted_t> } },
        { "MaaToolkitWaitForFindDeviceToComplete", { &MaaToolkitWaitForFindDeviceToComplete_Wrapper, &lhg::input_helper<__MaaToolkitWaitForFindDeviceToComplete_t>, &lhg::output_helper<__MaaToolkitWaitForFindDeviceToComplete_t> } },
        { "MaaToolkitGetDeviceCount", { &MaaToolkitGetDeviceCount_Wrapper, &lhg::input_helper<__MaaToolkitGetDeviceCount_t>, &lhg::output_helper<__MaaToolkitGetDeviceCount_t> } },
        { "MaaToolkitGetDeviceName", { &MaaToolkitGetDeviceName_Wrapper, &lhg::input_helper<__MaaToolkitGetDeviceName_t>, &lhg::output_helper<__MaaToolkitGetDeviceName_t> } },
        { "MaaToolkitGetDeviceAdbPath", { &MaaToolkitGetDeviceAdbPath_Wrapper, &lhg::input_helper<__MaaToolkitGetDeviceAdbPath_t>, &lhg::output_helper<__MaaToolkitGetDeviceAdbPath_t> } },
        { "MaaToolkitGetDeviceAdbSerial", { &MaaToolkitGetDeviceAdbSerial_Wrapper, &lhg::input_helper<__MaaToolkitGetDeviceAdbSerial_t>, &lhg::output_helper<__MaaToolkitGetDeviceAdbSerial_t> } },
        { "MaaToolkitGetDeviceAdbControllerType", { &MaaToolkitGetDeviceAdbControllerType_Wrapper, &lhg::input_helper<__MaaToolkitGetDeviceAdbControllerType_t>, &lhg::output_helper<__MaaToolkitGetDeviceAdbControllerType_t> } },
        { "MaaToolkitGetDeviceAdbConfig", { &MaaToolkitGetDeviceAdbConfig_Wrapper, &lhg::input_helper<__MaaToolkitGetDeviceAdbConfig_t>, &lhg::output_helper<__MaaToolkitGetDeviceAdbConfig_t> } },
        { "MaaToolkitRegisterCustomRecognizerExecutor", { &MaaToolkitRegisterCustomRecognizerExecutor_Wrapper, &lhg::input_helper<__MaaToolkitRegisterCustomRecognizerExecutor_t>, &lhg::output_helper<__MaaToolkitRegisterCustomRecognizerExecutor_t> } },
        { "MaaToolkitUnregisterCustomRecognizerExecutor", { &MaaToolkitUnregisterCustomRecognizerExecutor_Wrapper, &lhg::input_helper<__MaaToolkitUnregisterCustomRecognizerExecutor_t>, &lhg::output_helper<__MaaToolkitUnregisterCustomRecognizerExecutor_t> } },
        { "MaaToolkitRegisterCustomActionExecutor", { &MaaToolkitRegisterCustomActionExecutor_Wrapper, &lhg::input_helper<__MaaToolkitRegisterCustomActionExecutor_t>, &lhg::output_helper<__MaaToolkitRegisterCustomActionExecutor_t> } },
        { "MaaToolkitUnregisterCustomActionExecutor", { &MaaToolkitUnregisterCustomActionExecutor_Wrapper, &lhg::input_helper<__MaaToolkitUnregisterCustomActionExecutor_t>, &lhg::output_helper<__MaaToolkitUnregisterCustomActionExecutor_t> } },
        { "MaaToolkitFindWindow", { &MaaToolkitFindWindow_Wrapper, &lhg::input_helper<__MaaToolkitFindWindow_t>, &lhg::output_helper<__MaaToolkitFindWindow_t> } },
        { "MaaToolkitSearchWindow", { &MaaToolkitSearchWindow_Wrapper, &lhg::input_helper<__MaaToolkitSearchWindow_t>, &lhg::output_helper<__MaaToolkitSearchWindow_t> } },
        { "MaaToolkitGetWindow", { &MaaToolkitGetWindow_Wrapper, &lhg::input_helper<__MaaToolkitGetWindow_t>, &lhg::output_helper<__MaaToolkitGetWindow_t> } },
        { "MaaToolkitGetCursorWindow", { &MaaToolkitGetCursorWindow_Wrapper, &lhg::input_helper<__MaaToolkitGetCursorWindow_t>, &lhg::output_helper<__MaaToolkitGetCursorWindow_t> } },
        { "MaaControllerSetOptionString", { &MaaControllerSetOptionString_Wrapper, &lhg::input_helper<__MaaControllerSetOptionString_t>, &lhg::output_helper<__MaaControllerSetOptionString_t> } },
        { "MaaControllerSetOptionInteger", { &MaaControllerSetOptionInteger_Wrapper, &lhg::input_helper<__MaaControllerSetOptionInteger_t>, &lhg::output_helper<__MaaControllerSetOptionInteger_t> } },
        { "MaaControllerSetOptionBoolean", { &MaaControllerSetOptionBoolean_Wrapper, &lhg::input_helper<__MaaControllerSetOptionBoolean_t>, &lhg::output_helper<__MaaControllerSetOptionBoolean_t> } },
        { "MaaSetGlobalOptionString", { &MaaSetGlobalOptionString_Wrapper, &lhg::input_helper<__MaaSetGlobalOptionString_t>, &lhg::output_helper<__MaaSetGlobalOptionString_t> } },
        { "MaaSetGlobalOptionInteger", { &MaaSetGlobalOptionInteger_Wrapper, &lhg::input_helper<__MaaSetGlobalOptionInteger_t>, &lhg::output_helper<__MaaSetGlobalOptionInteger_t> } },
        { "MaaSetGlobalOptionBoolean", { &MaaSetGlobalOptionBoolean_Wrapper, &lhg::input_helper<__MaaSetGlobalOptionBoolean_t>, &lhg::output_helper<__MaaSetGlobalOptionBoolean_t> } },
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
