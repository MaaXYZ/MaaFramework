// clang-format off

#include "LHGUtils.h"
#define LHG_PROCESS
#define LHG_BUILD

// LHG SEC BEGIN lhg.include
#include "include.h"
#include "base64.hpp"
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
        static constexpr const char* const schema = "{ \"type\": \"string\" }";
    };

    template <>
    struct schema_t<MaaRect *>
    {
        static constexpr const char* const schema = "{ \"type\": \"object\", \"required\": [ \"x\", \"y\", \"width\", \"height\" ], \"properties\": { \"x\": { \"type\": \"number\" }, \"y\": { \"type\": \"number\" }, \"width\": { \"type\": \"number\" }, \"height\": { \"type\": \"number\" } } }";
    };
}
// LHG SEC END

static lhg::callback_manager<void (*)(const char *, const char *, void *)> MaaAPICallback__Manager;

static lhg::callback_manager<unsigned char (*)(MaaSyncContextAPI *, const char *, const char *, MaaRect *, const char *, void *)> CustomActionRun__Manager;

static lhg::callback_manager<void (*)(void *)> CustomActionStop__Manager;

static lhg::callback_manager<unsigned char (*)(MaaSyncContextAPI *, MaaImageBuffer *, const char *, const char *, void *, MaaRect *, MaaStringBuffer *)> CustomRecognizerAnalyze__Manager;

struct MaaAdbControllerCreate__ft {
    struct adb_path__at {
        using type = const char *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "adb_path";
    };
    struct address__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "address";
    };
    struct type__at {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "type";
    };
    struct config__at {
        using type = const char *;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "config";
    };
    struct callback__at {
        using type = void (*)(const char *, const char *, void *);
        static constexpr size_t index = 4;
        static constexpr const char* const name = "callback";
    };
    struct callback_arg__at {
        using type = void *;
        static constexpr size_t index = 5;
        static constexpr const char* const name = "callback_arg";
    };
    using arguments_t = std::tuple<adb_path__at, address__at, type__at, config__at, callback__at, callback_arg__at>;
    using return_t = MaaControllerAPI *;
};

template<>
struct lhg::is_callback<MaaAdbControllerCreate__ft::callback__at>
{
    static constexpr const bool value = true;
    static constexpr const char* const name = "MaaAPICallback";
    static constexpr const size_t context = 2;
    using traits = lhg::func_traits<typename MaaAdbControllerCreate__ft::callback__at::type>;
    static decltype(MaaAPICallback__Manager) &manager; 
};
decltype(MaaAPICallback__Manager)& lhg::is_callback<MaaAdbControllerCreate__ft::callback__at>::manager = MaaAPICallback__Manager;

template<>
struct lhg::is_callback_context<MaaAdbControllerCreate__ft::callback_arg__at>
{
    static constexpr const bool value = true;
    using callback_arg_tag = MaaAdbControllerCreate__ft::callback__at;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaAdbControllerCreate.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaAdbControllerCreateV2__ft {
    struct adb_path__at {
        using type = const char *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "adb_path";
    };
    struct address__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "address";
    };
    struct type__at {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "type";
    };
    struct config__at {
        using type = const char *;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "config";
    };
    struct agent_path__at {
        using type = const char *;
        static constexpr size_t index = 4;
        static constexpr const char* const name = "agent_path";
    };
    struct callback__at {
        using type = void (*)(const char *, const char *, void *);
        static constexpr size_t index = 5;
        static constexpr const char* const name = "callback";
    };
    struct callback_arg__at {
        using type = void *;
        static constexpr size_t index = 6;
        static constexpr const char* const name = "callback_arg";
    };
    using arguments_t = std::tuple<adb_path__at, address__at, type__at, config__at, agent_path__at, callback__at, callback_arg__at>;
    using return_t = MaaControllerAPI *;
};

template<>
struct lhg::is_callback<MaaAdbControllerCreateV2__ft::callback__at>
{
    static constexpr const bool value = true;
    static constexpr const char* const name = "MaaAPICallback";
    static constexpr const size_t context = 2;
    using traits = lhg::func_traits<typename MaaAdbControllerCreateV2__ft::callback__at::type>;
    static decltype(MaaAPICallback__Manager) &manager; 
};
decltype(MaaAPICallback__Manager)& lhg::is_callback<MaaAdbControllerCreateV2__ft::callback__at>::manager = MaaAPICallback__Manager;

template<>
struct lhg::is_callback_context<MaaAdbControllerCreateV2__ft::callback_arg__at>
{
    static constexpr const bool value = true;
    using callback_arg_tag = MaaAdbControllerCreateV2__ft::callback__at;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaAdbControllerCreateV2.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaThriftControllerCreate__ft {
    struct type__at {
        using type = int;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "type";
    };
    struct host__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "host";
    };
    struct port__at {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "port";
    };
    struct config__at {
        using type = const char *;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "config";
    };
    struct callback__at {
        using type = void (*)(const char *, const char *, void *);
        static constexpr size_t index = 4;
        static constexpr const char* const name = "callback";
    };
    struct callback_arg__at {
        using type = void *;
        static constexpr size_t index = 5;
        static constexpr const char* const name = "callback_arg";
    };
    using arguments_t = std::tuple<type__at, host__at, port__at, config__at, callback__at, callback_arg__at>;
    using return_t = MaaControllerAPI *;
};

template<>
struct lhg::is_callback<MaaThriftControllerCreate__ft::callback__at>
{
    static constexpr const bool value = true;
    static constexpr const char* const name = "MaaAPICallback";
    static constexpr const size_t context = 2;
    using traits = lhg::func_traits<typename MaaThriftControllerCreate__ft::callback__at::type>;
    static decltype(MaaAPICallback__Manager) &manager; 
};
decltype(MaaAPICallback__Manager)& lhg::is_callback<MaaThriftControllerCreate__ft::callback__at>::manager = MaaAPICallback__Manager;

template<>
struct lhg::is_callback_context<MaaThriftControllerCreate__ft::callback_arg__at>
{
    static constexpr const bool value = true;
    using callback_arg_tag = MaaThriftControllerCreate__ft::callback__at;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaThriftControllerCreate.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaDbgControllerCreate__ft {
    struct read_path__at {
        using type = const char *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "read_path";
    };
    struct write_path__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "write_path";
    };
    struct type__at {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "type";
    };
    struct config__at {
        using type = const char *;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "config";
    };
    struct callback__at {
        using type = void (*)(const char *, const char *, void *);
        static constexpr size_t index = 4;
        static constexpr const char* const name = "callback";
    };
    struct callback_arg__at {
        using type = void *;
        static constexpr size_t index = 5;
        static constexpr const char* const name = "callback_arg";
    };
    using arguments_t = std::tuple<read_path__at, write_path__at, type__at, config__at, callback__at, callback_arg__at>;
    using return_t = MaaControllerAPI *;
};

template<>
struct lhg::is_callback<MaaDbgControllerCreate__ft::callback__at>
{
    static constexpr const bool value = true;
    static constexpr const char* const name = "MaaAPICallback";
    static constexpr const size_t context = 2;
    using traits = lhg::func_traits<typename MaaDbgControllerCreate__ft::callback__at::type>;
    static decltype(MaaAPICallback__Manager) &manager; 
};
decltype(MaaAPICallback__Manager)& lhg::is_callback<MaaDbgControllerCreate__ft::callback__at>::manager = MaaAPICallback__Manager;

template<>
struct lhg::is_callback_context<MaaDbgControllerCreate__ft::callback_arg__at>
{
    static constexpr const bool value = true;
    using callback_arg_tag = MaaDbgControllerCreate__ft::callback__at;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaDbgControllerCreate.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerDestroy__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    using arguments_t = std::tuple<ctrl__at>;
    using return_t = void;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerDestroy.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerPostConnection__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    using arguments_t = std::tuple<ctrl__at>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostConnection.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerPostClick__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct x__at {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "x";
    };
    struct y__at {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "y";
    };
    using arguments_t = std::tuple<ctrl__at, x__at, y__at>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostClick.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerPostSwipe__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct x1__at {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "x1";
    };
    struct y1__at {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "y1";
    };
    struct x2__at {
        using type = int;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "x2";
    };
    struct y2__at {
        using type = int;
        static constexpr size_t index = 4;
        static constexpr const char* const name = "y2";
    };
    struct duration__at {
        using type = int;
        static constexpr size_t index = 5;
        static constexpr const char* const name = "duration";
    };
    using arguments_t = std::tuple<ctrl__at, x1__at, y1__at, x2__at, y2__at, duration__at>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostSwipe.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerPostPressKey__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct keycode__at {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "keycode";
    };
    using arguments_t = std::tuple<ctrl__at, keycode__at>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostPressKey.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerPostInputText__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct text__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "text";
    };
    using arguments_t = std::tuple<ctrl__at, text__at>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostInputText.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerPostTouchDown__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct contact__at {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "contact";
    };
    struct x__at {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "x";
    };
    struct y__at {
        using type = int;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "y";
    };
    struct pressure__at {
        using type = int;
        static constexpr size_t index = 4;
        static constexpr const char* const name = "pressure";
    };
    using arguments_t = std::tuple<ctrl__at, contact__at, x__at, y__at, pressure__at>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostTouchDown.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerPostTouchMove__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct contact__at {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "contact";
    };
    struct x__at {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "x";
    };
    struct y__at {
        using type = int;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "y";
    };
    struct pressure__at {
        using type = int;
        static constexpr size_t index = 4;
        static constexpr const char* const name = "pressure";
    };
    using arguments_t = std::tuple<ctrl__at, contact__at, x__at, y__at, pressure__at>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostTouchMove.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerPostTouchUp__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct contact__at {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "contact";
    };
    using arguments_t = std::tuple<ctrl__at, contact__at>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostTouchUp.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerPostScreencap__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    using arguments_t = std::tuple<ctrl__at>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerPostScreencap.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerStatus__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct id__at {
        using type = long long;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "id";
    };
    using arguments_t = std::tuple<ctrl__at, id__at>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerStatus.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerWait__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct id__at {
        using type = long long;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "id";
    };
    using arguments_t = std::tuple<ctrl__at, id__at>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerWait.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerConnected__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    using arguments_t = std::tuple<ctrl__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerConnected.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerGetImage__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct buffer__at {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "buffer";
    };
    using arguments_t = std::tuple<ctrl__at, buffer__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerGetImage.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerGetUUID__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct buffer__at {
        using type = MaaStringBuffer *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "buffer";
    };
    using arguments_t = std::tuple<ctrl__at, buffer__at>;
    using return_t = unsigned char;
};

template<>
struct lhg::is_output<MaaControllerGetUUID__ft::buffer__at>
{
    static constexpr const bool value = true;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerGetUUID.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaCreate__ft {
    struct callback__at {
        using type = void (*)(const char *, const char *, void *);
        static constexpr size_t index = 0;
        static constexpr const char* const name = "callback";
    };
    struct callback_arg__at {
        using type = void *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "callback_arg";
    };
    using arguments_t = std::tuple<callback__at, callback_arg__at>;
    using return_t = MaaInstanceAPI *;
};

template<>
struct lhg::is_callback<MaaCreate__ft::callback__at>
{
    static constexpr const bool value = true;
    static constexpr const char* const name = "MaaAPICallback";
    static constexpr const size_t context = 2;
    using traits = lhg::func_traits<typename MaaCreate__ft::callback__at::type>;
    static decltype(MaaAPICallback__Manager) &manager; 
};
decltype(MaaAPICallback__Manager)& lhg::is_callback<MaaCreate__ft::callback__at>::manager = MaaAPICallback__Manager;

template<>
struct lhg::is_callback_context<MaaCreate__ft::callback_arg__at>
{
    static constexpr const bool value = true;
    using callback_arg_tag = MaaCreate__ft::callback__at;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaCreate.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaDestroy__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<inst__at>;
    using return_t = void;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaDestroy.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaBindResource__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    struct res__at {
        using type = MaaResourceAPI *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "res";
    };
    using arguments_t = std::tuple<inst__at, res__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaBindResource.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaBindController__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "ctrl";
    };
    using arguments_t = std::tuple<inst__at, ctrl__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaBindController.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaInited__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<inst__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaInited.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaUnregisterCustomRecognizer__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    struct name__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "name";
    };
    using arguments_t = std::tuple<inst__at, name__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaUnregisterCustomRecognizer.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaClearCustomRecognizer__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<inst__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaClearCustomRecognizer.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaUnregisterCustomAction__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    struct name__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "name";
    };
    using arguments_t = std::tuple<inst__at, name__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaUnregisterCustomAction.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaClearCustomAction__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<inst__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaClearCustomAction.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaPostTask__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    struct entry__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "entry";
    };
    struct param__at {
        using type = const char *;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "param";
    };
    using arguments_t = std::tuple<inst__at, entry__at, param__at>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaPostTask.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaSetTaskParam__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    struct id__at {
        using type = long long;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "id";
    };
    struct param__at {
        using type = const char *;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "param";
    };
    using arguments_t = std::tuple<inst__at, id__at, param__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaSetTaskParam.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaTaskStatus__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    struct id__at {
        using type = long long;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "id";
    };
    using arguments_t = std::tuple<inst__at, id__at>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaTaskStatus.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaWaitTask__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    struct id__at {
        using type = long long;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "id";
    };
    using arguments_t = std::tuple<inst__at, id__at>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaWaitTask.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaTaskAllFinished__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<inst__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaTaskAllFinished.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaPostStop__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<inst__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaPostStop.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaStop__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<inst__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaStop.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaGetResource__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<inst__at>;
    using return_t = MaaResourceAPI *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaGetResource.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaGetController__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    using arguments_t = std::tuple<inst__at>;
    using return_t = MaaControllerAPI *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaGetController.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaResourceCreate__ft {
    struct callback__at {
        using type = void (*)(const char *, const char *, void *);
        static constexpr size_t index = 0;
        static constexpr const char* const name = "callback";
    };
    struct callback_arg__at {
        using type = void *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "callback_arg";
    };
    using arguments_t = std::tuple<callback__at, callback_arg__at>;
    using return_t = MaaResourceAPI *;
};

template<>
struct lhg::is_callback<MaaResourceCreate__ft::callback__at>
{
    static constexpr const bool value = true;
    static constexpr const char* const name = "MaaAPICallback";
    static constexpr const size_t context = 2;
    using traits = lhg::func_traits<typename MaaResourceCreate__ft::callback__at::type>;
    static decltype(MaaAPICallback__Manager) &manager; 
};
decltype(MaaAPICallback__Manager)& lhg::is_callback<MaaResourceCreate__ft::callback__at>::manager = MaaAPICallback__Manager;

template<>
struct lhg::is_callback_context<MaaResourceCreate__ft::callback_arg__at>
{
    static constexpr const bool value = true;
    using callback_arg_tag = MaaResourceCreate__ft::callback__at;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaResourceCreate.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaResourceDestroy__ft {
    struct res__at {
        using type = MaaResourceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "res";
    };
    using arguments_t = std::tuple<res__at>;
    using return_t = void;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaResourceDestroy.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaResourcePostPath__ft {
    struct res__at {
        using type = MaaResourceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "res";
    };
    struct path__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "path";
    };
    using arguments_t = std::tuple<res__at, path__at>;
    using return_t = long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaResourcePostPath.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaResourceStatus__ft {
    struct res__at {
        using type = MaaResourceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "res";
    };
    struct id__at {
        using type = long long;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "id";
    };
    using arguments_t = std::tuple<res__at, id__at>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaResourceStatus.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaResourceWait__ft {
    struct res__at {
        using type = MaaResourceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "res";
    };
    struct id__at {
        using type = long long;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "id";
    };
    using arguments_t = std::tuple<res__at, id__at>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaResourceWait.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaResourceLoaded__ft {
    struct res__at {
        using type = MaaResourceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "res";
    };
    using arguments_t = std::tuple<res__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaResourceLoaded.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaResourceGetHash__ft {
    struct res__at {
        using type = MaaResourceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "res";
    };
    struct buffer__at {
        using type = MaaStringBuffer *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "buffer";
    };
    using arguments_t = std::tuple<res__at, buffer__at>;
    using return_t = unsigned char;
};

template<>
struct lhg::is_output<MaaResourceGetHash__ft::buffer__at>
{
    static constexpr const bool value = true;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaResourceGetHash.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaResourceGetTaskList__ft {
    struct res__at {
        using type = MaaResourceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "res";
    };
    struct buffer__at {
        using type = MaaStringBuffer *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "buffer";
    };
    using arguments_t = std::tuple<res__at, buffer__at>;
    using return_t = unsigned char;
};

template<>
struct lhg::is_output<MaaResourceGetTaskList__ft::buffer__at>
{
    static constexpr const bool value = true;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaResourceGetTaskList.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaCreateImageBuffer__ft {
    using arguments_t = void;
    using return_t = MaaImageBuffer *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaCreateImageBuffer.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaDestroyImageBuffer__ft {
    struct handle__at {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    using arguments_t = std::tuple<handle__at>;
    using return_t = void;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaDestroyImageBuffer.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaIsImageEmpty__ft {
    struct handle__at {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    using arguments_t = std::tuple<handle__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaIsImageEmpty.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaClearImage__ft {
    struct handle__at {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    using arguments_t = std::tuple<handle__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaClearImage.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaGetImageWidth__ft {
    struct handle__at {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    using arguments_t = std::tuple<handle__at>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaGetImageWidth.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaGetImageHeight__ft {
    struct handle__at {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    using arguments_t = std::tuple<handle__at>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaGetImageHeight.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaGetImageType__ft {
    struct handle__at {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    using arguments_t = std::tuple<handle__at>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaGetImageType.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaGetImageEncoded__ft {
    struct handle__at {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    using arguments_t = std::tuple<handle__at>;
    using return_t = unsigned char *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaGetImageEncoded.tag
template<>
struct ret_schema<MaaGetImageEncoded__ft>
{
    static constexpr const char* const schema = "{ \"type\": \"string\" }";
};
// LHG SEC END
}

struct MaaSetImageEncoded__ft {
    struct handle__at {
        using type = MaaImageBuffer *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    struct data__at {
        using type = unsigned char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "data";
    };
    struct size__at {
        using type = unsigned long long;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "size";
    };
    using arguments_t = std::tuple<handle__at, data__at, size__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaSetImageEncoded.tag
template<>
struct arg_schema<MaaSetImageEncoded__ft::data__at>
{
    static constexpr const char *const schema = "{ \"type\": \"string\" }";
};

template<>
struct shown_in_schema<MaaSetImageEncoded__ft::size__at>
{
    static constexpr const bool value = false;
};
// LHG SEC END
}

struct MaaVersion__ft {
    using arguments_t = void;
    using return_t = const char *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaVersion.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitInit__ft {
    using arguments_t = void;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitInit.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitUninit__ft {
    using arguments_t = void;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitUninit.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitFindDevice__ft {
    using arguments_t = void;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitFindDevice.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitFindDeviceWithAdb__ft {
    struct adb_path__at {
        using type = const char *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "adb_path";
    };
    using arguments_t = std::tuple<adb_path__at>;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitFindDeviceWithAdb.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitPostFindDevice__ft {
    using arguments_t = void;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitPostFindDevice.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitPostFindDeviceWithAdb__ft {
    struct adb_path__at {
        using type = const char *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "adb_path";
    };
    using arguments_t = std::tuple<adb_path__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitPostFindDeviceWithAdb.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitIsFindDeviceCompleted__ft {
    using arguments_t = void;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitIsFindDeviceCompleted.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitWaitForFindDeviceToComplete__ft {
    using arguments_t = void;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitWaitForFindDeviceToComplete.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitGetDeviceCount__ft {
    using arguments_t = void;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceCount.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitGetDeviceName__ft {
    struct index__at {
        using type = unsigned long long;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "index";
    };
    using arguments_t = std::tuple<index__at>;
    using return_t = const char *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceName.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitGetDeviceAdbPath__ft {
    struct index__at {
        using type = unsigned long long;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "index";
    };
    using arguments_t = std::tuple<index__at>;
    using return_t = const char *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceAdbPath.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitGetDeviceAdbSerial__ft {
    struct index__at {
        using type = unsigned long long;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "index";
    };
    using arguments_t = std::tuple<index__at>;
    using return_t = const char *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceAdbSerial.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitGetDeviceAdbControllerType__ft {
    struct index__at {
        using type = unsigned long long;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "index";
    };
    using arguments_t = std::tuple<index__at>;
    using return_t = int;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceAdbControllerType.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitGetDeviceAdbConfig__ft {
    struct index__at {
        using type = unsigned long long;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "index";
    };
    using arguments_t = std::tuple<index__at>;
    using return_t = const char *;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceAdbConfig.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitRegisterCustomRecognizerExecutor__ft {
    struct handle__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    struct recognizer_name__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "recognizer_name";
    };
    struct recognizer_exec_path__at {
        using type = const char *;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "recognizer_exec_path";
    };
    struct recognizer_exec_param_json__at {
        using type = const char *;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "recognizer_exec_param_json";
    };
    using arguments_t = std::tuple<handle__at, recognizer_name__at, recognizer_exec_path__at, recognizer_exec_param_json__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitRegisterCustomRecognizerExecutor.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitUnregisterCustomRecognizerExecutor__ft {
    struct handle__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    struct recognizer_name__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "recognizer_name";
    };
    using arguments_t = std::tuple<handle__at, recognizer_name__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitUnregisterCustomRecognizerExecutor.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitRegisterCustomActionExecutor__ft {
    struct handle__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    struct action_name__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "action_name";
    };
    struct action_exec_path__at {
        using type = const char *;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "action_exec_path";
    };
    struct action_exec_param_json__at {
        using type = const char *;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "action_exec_param_json";
    };
    using arguments_t = std::tuple<handle__at, action_name__at, action_exec_path__at, action_exec_param_json__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitRegisterCustomActionExecutor.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitUnregisterCustomActionExecutor__ft {
    struct handle__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "handle";
    };
    struct action_name__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "action_name";
    };
    using arguments_t = std::tuple<handle__at, action_name__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitUnregisterCustomActionExecutor.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitFindWindow__ft {
    struct class_name__at {
        using type = const char *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "class_name";
    };
    struct window_name__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "window_name";
    };
    using arguments_t = std::tuple<class_name__at, window_name__at>;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitFindWindow.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitSearchWindow__ft {
    struct class_name__at {
        using type = const char *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "class_name";
    };
    struct window_name__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "window_name";
    };
    using arguments_t = std::tuple<class_name__at, window_name__at>;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitSearchWindow.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitGetWindow__ft {
    struct index__at {
        using type = unsigned long long;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "index";
    };
    using arguments_t = std::tuple<index__at>;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetWindow.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitGetCursorWindow__ft {
    using arguments_t = void;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetCursorWindow.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitGetDesktopWindow__ft {
    using arguments_t = void;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetDesktopWindow.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaToolkitGetForegroundWindow__ft {
    using arguments_t = void;
    using return_t = unsigned long long;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaToolkitGetForegroundWindow.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerSetOptionString__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct key__at {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "key";
    };
    struct value__at {
        using type = const char *;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "value";
    };
    using arguments_t = std::tuple<ctrl__at, key__at, value__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerSetOptionString.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerSetOptionInteger__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct key__at {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "key";
    };
    struct value__at {
        using type = int;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "value";
    };
    using arguments_t = std::tuple<ctrl__at, key__at, value__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerSetOptionInteger.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaControllerSetOptionBoolean__ft {
    struct ctrl__at {
        using type = MaaControllerAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "ctrl";
    };
    struct key__at {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "key";
    };
    struct value__at {
        using type = bool;
        static constexpr size_t index = 2;
        static constexpr const char* const name = "value";
    };
    using arguments_t = std::tuple<ctrl__at, key__at, value__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaControllerSetOptionBoolean.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaSetGlobalOptionString__ft {
    struct key__at {
        using type = int;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "key";
    };
    struct value__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "value";
    };
    using arguments_t = std::tuple<key__at, value__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaSetGlobalOptionString.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaSetGlobalOptionInteger__ft {
    struct key__at {
        using type = int;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "key";
    };
    struct value__at {
        using type = int;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "value";
    };
    using arguments_t = std::tuple<key__at, value__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaSetGlobalOptionInteger.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaSetGlobalOptionBoolean__ft {
    struct key__at {
        using type = int;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "key";
    };
    struct value__at {
        using type = bool;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "value";
    };
    using arguments_t = std::tuple<key__at, value__at>;
    using return_t = unsigned char;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaSetGlobalOptionBoolean.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaRegisterCustomRecognizerImpl__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    struct name__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "name";
    };
    struct analyze__at {
        using type = unsigned char (*)(MaaSyncContextAPI *, MaaImageBuffer *, const char *, const char *, void *, MaaRect *, MaaStringBuffer *);
        static constexpr size_t index = 2;
        static constexpr const char* const name = "analyze";
    };
    struct analyze_arg__at {
        using type = void *;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "analyze_arg";
    };
    using arguments_t = std::tuple<inst__at, name__at, analyze__at, analyze_arg__at>;
    using return_t = unsigned char;
};

template<>
struct lhg::is_callback<MaaRegisterCustomRecognizerImpl__ft::analyze__at>
{
    static constexpr const bool value = true;
    static constexpr const char* const name = "CustomRecognizerAnalyze";
    static constexpr const size_t context = 4;
    using traits = lhg::func_traits<typename MaaRegisterCustomRecognizerImpl__ft::analyze__at::type>;
    static decltype(CustomRecognizerAnalyze__Manager) &manager; 
};
decltype(CustomRecognizerAnalyze__Manager)& lhg::is_callback<MaaRegisterCustomRecognizerImpl__ft::analyze__at>::manager = CustomRecognizerAnalyze__Manager;

template<>
struct lhg::is_callback_context<MaaRegisterCustomRecognizerImpl__ft::analyze_arg__at>
{
    static constexpr const bool value = true;
    using callback_arg_tag = MaaRegisterCustomRecognizerImpl__ft::analyze__at;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaRegisterCustomRecognizerImpl.tag
// LHG SEC DEF

// LHG SEC END
}

struct MaaRegisterCustomActionImpl__ft {
    struct inst__at {
        using type = MaaInstanceAPI *;
        static constexpr size_t index = 0;
        static constexpr const char* const name = "inst";
    };
    struct name__at {
        using type = const char *;
        static constexpr size_t index = 1;
        static constexpr const char* const name = "name";
    };
    struct run__at {
        using type = unsigned char (*)(MaaSyncContextAPI *, const char *, const char *, MaaRect *, const char *, void *);
        static constexpr size_t index = 2;
        static constexpr const char* const name = "run";
    };
    struct run_arg__at {
        using type = void *;
        static constexpr size_t index = 3;
        static constexpr const char* const name = "run_arg";
    };
    struct stop__at {
        using type = void (*)(void *);
        static constexpr size_t index = 4;
        static constexpr const char* const name = "stop";
    };
    struct stop_arg__at {
        using type = void *;
        static constexpr size_t index = 5;
        static constexpr const char* const name = "stop_arg";
    };
    using arguments_t = std::tuple<inst__at, name__at, run__at, run_arg__at, stop__at, stop_arg__at>;
    using return_t = unsigned char;
};

template<>
struct lhg::is_callback<MaaRegisterCustomActionImpl__ft::run__at>
{
    static constexpr const bool value = true;
    static constexpr const char* const name = "CustomActionRun";
    static constexpr const size_t context = 5;
    using traits = lhg::func_traits<typename MaaRegisterCustomActionImpl__ft::run__at::type>;
    static decltype(CustomActionRun__Manager) &manager; 
};
decltype(CustomActionRun__Manager)& lhg::is_callback<MaaRegisterCustomActionImpl__ft::run__at>::manager = CustomActionRun__Manager;

template<>
struct lhg::is_callback_context<MaaRegisterCustomActionImpl__ft::run_arg__at>
{
    static constexpr const bool value = true;
    using callback_arg_tag = MaaRegisterCustomActionImpl__ft::run__at;
};

template<>
struct lhg::is_callback<MaaRegisterCustomActionImpl__ft::stop__at>
{
    static constexpr const bool value = true;
    static constexpr const char* const name = "CustomActionStop";
    static constexpr const size_t context = 0;
    using traits = lhg::func_traits<typename MaaRegisterCustomActionImpl__ft::stop__at::type>;
    static decltype(CustomActionStop__Manager) &manager; 
};
decltype(CustomActionStop__Manager)& lhg::is_callback<MaaRegisterCustomActionImpl__ft::stop__at>::manager = CustomActionStop__Manager;

template<>
struct lhg::is_callback_context<MaaRegisterCustomActionImpl__ft::stop_arg__at>
{
    static constexpr const bool value = true;
    using callback_arg_tag = MaaRegisterCustomActionImpl__ft::stop__at;
};

namespace lhg {
// LHG SEC BEGIN lhg.custom.MaaRegisterCustomActionImpl.tag
// LHG SEC DEF

// LHG SEC END
}

static lhg::opaque_manager<MaaControllerAPI *> MaaControllerAPI__OpaqueManager;

template<>
struct lhg::schema_t<MaaControllerAPI *>
{
    static constexpr const char* const schema = "{\"type\":\"string\",\"title\":\"MaaControllerAPI\"}";
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
struct lhg::is_opaque_free<MaaControllerAPI *, MaaControllerDestroy__ft> {
    static constexpr const bool value = true;
};

template<>
struct lhg::is_opaque_non_alloc<MaaControllerAPI *, MaaGetController__ft> {
    static constexpr const bool value = true;
};

static lhg::opaque_manager<MaaResourceAPI *> MaaResourceAPI__OpaqueManager;

template<>
struct lhg::schema_t<MaaResourceAPI *>
{
    static constexpr const char* const schema = "{\"type\":\"string\",\"title\":\"MaaResourceAPI\"}";
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
struct lhg::is_opaque_free<MaaResourceAPI *, MaaResourceDestroy__ft> {
    static constexpr const bool value = true;
};

template<>
struct lhg::is_opaque_non_alloc<MaaResourceAPI *, MaaGetResource__ft> {
    static constexpr const bool value = true;
};

static lhg::opaque_manager<MaaInstanceAPI *> MaaInstanceAPI__OpaqueManager;

template<>
struct lhg::schema_t<MaaInstanceAPI *>
{
    static constexpr const char* const schema = "{\"type\":\"string\",\"title\":\"MaaInstanceAPI\"}";
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
struct lhg::is_opaque_free<MaaInstanceAPI *, MaaDestroy__ft> {
    static constexpr const bool value = true;
};

static lhg::opaque_manager<MaaImageBuffer *> MaaImageBuffer__OpaqueManager;

template<>
struct lhg::schema_t<MaaImageBuffer *>
{
    static constexpr const char* const schema = "{\"type\":\"string\",\"title\":\"MaaImageBuffer\"}";
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
struct lhg::is_opaque_free<MaaImageBuffer *, MaaDestroyImageBuffer__ft> {
    static constexpr const bool value = true;
};

static lhg::opaque_manager<MaaSyncContextAPI *> MaaSyncContextAPI__OpaqueManager;

template<>
struct lhg::schema_t<MaaSyncContextAPI *>
{
    static constexpr const char* const schema = "{\"type\":\"string\",\"title\":\"MaaSyncContextAPI\"}";
};

template<>
struct lhg::is_opaque<MaaSyncContextAPI *> {
    static constexpr const bool value = true;
    using type = MaaSyncContextAPI;
    static constexpr const char* const name = "MaaSyncContextAPI";
    static lhg::opaque_manager<MaaSyncContextAPI *>& manager;
};
lhg::opaque_manager<MaaSyncContextAPI *>& lhg::is_opaque<MaaSyncContextAPI *>::manager = MaaSyncContextAPI__OpaqueManager;

// LHG SEC BEGIN lhg.custom.MaaAdbControllerCreate.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaAdbControllerCreate_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaAdbControllerCreate__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaAdbControllerCreate__ft>::temp_type __temp;
    typename lhg::arg_set<MaaAdbControllerCreate__ft>::call_type __call;
    if (!lhg::perform_input<MaaAdbControllerCreate__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaAdbControllerCreate__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaAdbControllerCreate, __call);
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreate.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaAdbControllerCreate__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaAdbControllerCreateV2.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaAdbControllerCreateV2_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaAdbControllerCreateV2__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaAdbControllerCreateV2__ft>::temp_type __temp;
    typename lhg::arg_set<MaaAdbControllerCreateV2__ft>::call_type __call;
    if (!lhg::perform_input<MaaAdbControllerCreateV2__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaAdbControllerCreateV2__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaAdbControllerCreateV2, __call);
// LHG SEC BEGIN lhg.impl.MaaAdbControllerCreateV2.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaAdbControllerCreateV2__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaThriftControllerCreate.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaThriftControllerCreate_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaThriftControllerCreate__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaThriftControllerCreate__ft>::temp_type __temp;
    typename lhg::arg_set<MaaThriftControllerCreate__ft>::call_type __call;
    if (!lhg::perform_input<MaaThriftControllerCreate__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaThriftControllerCreate__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaThriftControllerCreate, __call);
// LHG SEC BEGIN lhg.impl.MaaThriftControllerCreate.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaThriftControllerCreate__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaDbgControllerCreate.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaDbgControllerCreate_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaDbgControllerCreate__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaDbgControllerCreate__ft>::temp_type __temp;
    typename lhg::arg_set<MaaDbgControllerCreate__ft>::call_type __call;
    if (!lhg::perform_input<MaaDbgControllerCreate__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaDbgControllerCreate__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaDbgControllerCreate, __call);
// LHG SEC BEGIN lhg.impl.MaaDbgControllerCreate.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaDbgControllerCreate__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerDestroy.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerDestroy_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerDestroy__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerDestroy__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerDestroy__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerDestroy__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerDestroy__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    std::apply(MaaControllerDestroy, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerDestroy.return
// LHG SEC DEF
    int __ret = 0;
// LHG SEC END
    return lhg::perform_output<MaaControllerDestroy__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostConnection.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostConnection_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerPostConnection__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerPostConnection__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerPostConnection__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerPostConnection__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerPostConnection__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostConnection, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostConnection.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerPostConnection__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostClick.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostClick_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerPostClick__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerPostClick__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerPostClick__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerPostClick__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerPostClick__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostClick, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostClick.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerPostClick__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostSwipe.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostSwipe_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerPostSwipe__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerPostSwipe__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerPostSwipe__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerPostSwipe__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerPostSwipe__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostSwipe, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostSwipe.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerPostSwipe__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostPressKey.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostPressKey_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerPostPressKey__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerPostPressKey__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerPostPressKey__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerPostPressKey__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerPostPressKey__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostPressKey, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostPressKey.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerPostPressKey__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostInputText.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostInputText_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerPostInputText__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerPostInputText__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerPostInputText__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerPostInputText__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerPostInputText__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostInputText, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostInputText.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerPostInputText__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostTouchDown.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostTouchDown_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerPostTouchDown__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerPostTouchDown__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerPostTouchDown__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerPostTouchDown__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerPostTouchDown__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostTouchDown, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchDown.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerPostTouchDown__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostTouchMove.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostTouchMove_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerPostTouchMove__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerPostTouchMove__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerPostTouchMove__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerPostTouchMove__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerPostTouchMove__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostTouchMove, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchMove.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerPostTouchMove__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostTouchUp.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostTouchUp_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerPostTouchUp__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerPostTouchUp__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerPostTouchUp__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerPostTouchUp__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerPostTouchUp__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostTouchUp, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostTouchUp.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerPostTouchUp__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerPostScreencap.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerPostScreencap_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerPostScreencap__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerPostScreencap__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerPostScreencap__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerPostScreencap__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerPostScreencap__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerPostScreencap, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerPostScreencap.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerPostScreencap__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerStatus.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerStatus_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerStatus__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerStatus__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerStatus__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerStatus__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerStatus__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerStatus, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerStatus.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerStatus__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerWait.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerWait_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerWait__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerWait__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerWait__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerWait__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerWait__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerWait, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerWait.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerWait__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerConnected.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerConnected_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerConnected__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerConnected__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerConnected__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerConnected__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerConnected__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerConnected, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerConnected.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerConnected__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerGetImage.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerGetImage_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerGetImage__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerGetImage__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerGetImage__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerGetImage__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerGetImage__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerGetImage, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerGetImage.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerGetImage__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerGetUUID.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerGetUUID_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerGetUUID__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerGetUUID__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerGetUUID__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerGetUUID__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerGetUUID__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerGetUUID, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerGetUUID.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerGetUUID__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaCreate.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaCreate_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaCreate__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaCreate__ft>::temp_type __temp;
    typename lhg::arg_set<MaaCreate__ft>::call_type __call;
    if (!lhg::perform_input<MaaCreate__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaCreate__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaCreate, __call);
// LHG SEC BEGIN lhg.impl.MaaCreate.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaCreate__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaDestroy.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaDestroy_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaDestroy__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaDestroy__ft>::temp_type __temp;
    typename lhg::arg_set<MaaDestroy__ft>::call_type __call;
    if (!lhg::perform_input<MaaDestroy__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaDestroy__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    std::apply(MaaDestroy, __call);
// LHG SEC BEGIN lhg.impl.MaaDestroy.return
// LHG SEC DEF
    int __ret = 0;
// LHG SEC END
    return lhg::perform_output<MaaDestroy__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaBindResource.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaBindResource_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaBindResource__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaBindResource__ft>::temp_type __temp;
    typename lhg::arg_set<MaaBindResource__ft>::call_type __call;
    if (!lhg::perform_input<MaaBindResource__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaBindResource__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaBindResource, __call);
// LHG SEC BEGIN lhg.impl.MaaBindResource.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaBindResource__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaBindController.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaBindController_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaBindController__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaBindController__ft>::temp_type __temp;
    typename lhg::arg_set<MaaBindController__ft>::call_type __call;
    if (!lhg::perform_input<MaaBindController__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaBindController__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaBindController, __call);
// LHG SEC BEGIN lhg.impl.MaaBindController.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaBindController__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaInited.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaInited_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaInited__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaInited__ft>::temp_type __temp;
    typename lhg::arg_set<MaaInited__ft>::call_type __call;
    if (!lhg::perform_input<MaaInited__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaInited__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaInited, __call);
// LHG SEC BEGIN lhg.impl.MaaInited.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaInited__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaUnregisterCustomRecognizer.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaUnregisterCustomRecognizer_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaUnregisterCustomRecognizer__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaUnregisterCustomRecognizer__ft>::temp_type __temp;
    typename lhg::arg_set<MaaUnregisterCustomRecognizer__ft>::call_type __call;
    if (!lhg::perform_input<MaaUnregisterCustomRecognizer__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaUnregisterCustomRecognizer__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaUnregisterCustomRecognizer, __call);
// LHG SEC BEGIN lhg.impl.MaaUnregisterCustomRecognizer.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaUnregisterCustomRecognizer__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaClearCustomRecognizer.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaClearCustomRecognizer_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaClearCustomRecognizer__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaClearCustomRecognizer__ft>::temp_type __temp;
    typename lhg::arg_set<MaaClearCustomRecognizer__ft>::call_type __call;
    if (!lhg::perform_input<MaaClearCustomRecognizer__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaClearCustomRecognizer__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaClearCustomRecognizer, __call);
// LHG SEC BEGIN lhg.impl.MaaClearCustomRecognizer.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaClearCustomRecognizer__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaUnregisterCustomAction.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaUnregisterCustomAction_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaUnregisterCustomAction__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaUnregisterCustomAction__ft>::temp_type __temp;
    typename lhg::arg_set<MaaUnregisterCustomAction__ft>::call_type __call;
    if (!lhg::perform_input<MaaUnregisterCustomAction__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaUnregisterCustomAction__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaUnregisterCustomAction, __call);
// LHG SEC BEGIN lhg.impl.MaaUnregisterCustomAction.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaUnregisterCustomAction__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaClearCustomAction.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaClearCustomAction_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaClearCustomAction__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaClearCustomAction__ft>::temp_type __temp;
    typename lhg::arg_set<MaaClearCustomAction__ft>::call_type __call;
    if (!lhg::perform_input<MaaClearCustomAction__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaClearCustomAction__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaClearCustomAction, __call);
// LHG SEC BEGIN lhg.impl.MaaClearCustomAction.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaClearCustomAction__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaPostTask.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaPostTask_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaPostTask__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaPostTask__ft>::temp_type __temp;
    typename lhg::arg_set<MaaPostTask__ft>::call_type __call;
    if (!lhg::perform_input<MaaPostTask__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaPostTask__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaPostTask, __call);
// LHG SEC BEGIN lhg.impl.MaaPostTask.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaPostTask__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaSetTaskParam.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaSetTaskParam_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaSetTaskParam__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaSetTaskParam__ft>::temp_type __temp;
    typename lhg::arg_set<MaaSetTaskParam__ft>::call_type __call;
    if (!lhg::perform_input<MaaSetTaskParam__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaSetTaskParam__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaSetTaskParam, __call);
// LHG SEC BEGIN lhg.impl.MaaSetTaskParam.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaSetTaskParam__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaTaskStatus.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaTaskStatus_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaTaskStatus__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaTaskStatus__ft>::temp_type __temp;
    typename lhg::arg_set<MaaTaskStatus__ft>::call_type __call;
    if (!lhg::perform_input<MaaTaskStatus__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaTaskStatus__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaTaskStatus, __call);
// LHG SEC BEGIN lhg.impl.MaaTaskStatus.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaTaskStatus__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaWaitTask.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaWaitTask_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaWaitTask__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaWaitTask__ft>::temp_type __temp;
    typename lhg::arg_set<MaaWaitTask__ft>::call_type __call;
    if (!lhg::perform_input<MaaWaitTask__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaWaitTask__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaWaitTask, __call);
// LHG SEC BEGIN lhg.impl.MaaWaitTask.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaWaitTask__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaTaskAllFinished.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaTaskAllFinished_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaTaskAllFinished__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaTaskAllFinished__ft>::temp_type __temp;
    typename lhg::arg_set<MaaTaskAllFinished__ft>::call_type __call;
    if (!lhg::perform_input<MaaTaskAllFinished__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaTaskAllFinished__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaTaskAllFinished, __call);
// LHG SEC BEGIN lhg.impl.MaaTaskAllFinished.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaTaskAllFinished__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaPostStop.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaPostStop_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaPostStop__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaPostStop__ft>::temp_type __temp;
    typename lhg::arg_set<MaaPostStop__ft>::call_type __call;
    if (!lhg::perform_input<MaaPostStop__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaPostStop__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaPostStop, __call);
// LHG SEC BEGIN lhg.impl.MaaPostStop.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaPostStop__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaStop.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaStop_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaStop__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaStop__ft>::temp_type __temp;
    typename lhg::arg_set<MaaStop__ft>::call_type __call;
    if (!lhg::perform_input<MaaStop__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaStop__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaStop, __call);
// LHG SEC BEGIN lhg.impl.MaaStop.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaStop__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaGetResource.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaGetResource_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaGetResource__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaGetResource__ft>::temp_type __temp;
    typename lhg::arg_set<MaaGetResource__ft>::call_type __call;
    if (!lhg::perform_input<MaaGetResource__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaGetResource__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaGetResource, __call);
// LHG SEC BEGIN lhg.impl.MaaGetResource.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaGetResource__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaGetController.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaGetController_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaGetController__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaGetController__ft>::temp_type __temp;
    typename lhg::arg_set<MaaGetController__ft>::call_type __call;
    if (!lhg::perform_input<MaaGetController__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaGetController__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaGetController, __call);
// LHG SEC BEGIN lhg.impl.MaaGetController.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaGetController__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaResourceCreate.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaResourceCreate_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaResourceCreate__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaResourceCreate__ft>::temp_type __temp;
    typename lhg::arg_set<MaaResourceCreate__ft>::call_type __call;
    if (!lhg::perform_input<MaaResourceCreate__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaResourceCreate__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaResourceCreate, __call);
// LHG SEC BEGIN lhg.impl.MaaResourceCreate.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaResourceCreate__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaResourceDestroy.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaResourceDestroy_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaResourceDestroy__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaResourceDestroy__ft>::temp_type __temp;
    typename lhg::arg_set<MaaResourceDestroy__ft>::call_type __call;
    if (!lhg::perform_input<MaaResourceDestroy__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaResourceDestroy__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    std::apply(MaaResourceDestroy, __call);
// LHG SEC BEGIN lhg.impl.MaaResourceDestroy.return
// LHG SEC DEF
    int __ret = 0;
// LHG SEC END
    return lhg::perform_output<MaaResourceDestroy__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaResourcePostPath.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaResourcePostPath_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaResourcePostPath__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaResourcePostPath__ft>::temp_type __temp;
    typename lhg::arg_set<MaaResourcePostPath__ft>::call_type __call;
    if (!lhg::perform_input<MaaResourcePostPath__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaResourcePostPath__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaResourcePostPath, __call);
// LHG SEC BEGIN lhg.impl.MaaResourcePostPath.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaResourcePostPath__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaResourceStatus.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaResourceStatus_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaResourceStatus__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaResourceStatus__ft>::temp_type __temp;
    typename lhg::arg_set<MaaResourceStatus__ft>::call_type __call;
    if (!lhg::perform_input<MaaResourceStatus__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaResourceStatus__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaResourceStatus, __call);
// LHG SEC BEGIN lhg.impl.MaaResourceStatus.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaResourceStatus__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaResourceWait.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaResourceWait_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaResourceWait__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaResourceWait__ft>::temp_type __temp;
    typename lhg::arg_set<MaaResourceWait__ft>::call_type __call;
    if (!lhg::perform_input<MaaResourceWait__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaResourceWait__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaResourceWait, __call);
// LHG SEC BEGIN lhg.impl.MaaResourceWait.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaResourceWait__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaResourceLoaded.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaResourceLoaded_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaResourceLoaded__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaResourceLoaded__ft>::temp_type __temp;
    typename lhg::arg_set<MaaResourceLoaded__ft>::call_type __call;
    if (!lhg::perform_input<MaaResourceLoaded__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaResourceLoaded__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaResourceLoaded, __call);
// LHG SEC BEGIN lhg.impl.MaaResourceLoaded.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaResourceLoaded__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaResourceGetHash.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaResourceGetHash_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaResourceGetHash__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaResourceGetHash__ft>::temp_type __temp;
    typename lhg::arg_set<MaaResourceGetHash__ft>::call_type __call;
    if (!lhg::perform_input<MaaResourceGetHash__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaResourceGetHash__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaResourceGetHash, __call);
// LHG SEC BEGIN lhg.impl.MaaResourceGetHash.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaResourceGetHash__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaResourceGetTaskList.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaResourceGetTaskList_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaResourceGetTaskList__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaResourceGetTaskList__ft>::temp_type __temp;
    typename lhg::arg_set<MaaResourceGetTaskList__ft>::call_type __call;
    if (!lhg::perform_input<MaaResourceGetTaskList__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaResourceGetTaskList__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaResourceGetTaskList, __call);
// LHG SEC BEGIN lhg.impl.MaaResourceGetTaskList.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaResourceGetTaskList__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaCreateImageBuffer.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaCreateImageBuffer_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaCreateImageBuffer__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaCreateImageBuffer__ft>::temp_type __temp;
    typename lhg::arg_set<MaaCreateImageBuffer__ft>::call_type __call;
    if (!lhg::perform_input<MaaCreateImageBuffer__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaCreateImageBuffer__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaCreateImageBuffer();
// LHG SEC BEGIN lhg.impl.MaaCreateImageBuffer.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaCreateImageBuffer__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaDestroyImageBuffer.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaDestroyImageBuffer_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaDestroyImageBuffer__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaDestroyImageBuffer__ft>::temp_type __temp;
    typename lhg::arg_set<MaaDestroyImageBuffer__ft>::call_type __call;
    if (!lhg::perform_input<MaaDestroyImageBuffer__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaDestroyImageBuffer__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    std::apply(MaaDestroyImageBuffer, __call);
// LHG SEC BEGIN lhg.impl.MaaDestroyImageBuffer.return
// LHG SEC DEF
    int __ret = 0;
// LHG SEC END
    return lhg::perform_output<MaaDestroyImageBuffer__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaIsImageEmpty.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaIsImageEmpty_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaIsImageEmpty__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaIsImageEmpty__ft>::temp_type __temp;
    typename lhg::arg_set<MaaIsImageEmpty__ft>::call_type __call;
    if (!lhg::perform_input<MaaIsImageEmpty__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaIsImageEmpty__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaIsImageEmpty, __call);
// LHG SEC BEGIN lhg.impl.MaaIsImageEmpty.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaIsImageEmpty__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaClearImage.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaClearImage_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaClearImage__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaClearImage__ft>::temp_type __temp;
    typename lhg::arg_set<MaaClearImage__ft>::call_type __call;
    if (!lhg::perform_input<MaaClearImage__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaClearImage__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaClearImage, __call);
// LHG SEC BEGIN lhg.impl.MaaClearImage.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaClearImage__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaGetImageWidth.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaGetImageWidth_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaGetImageWidth__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaGetImageWidth__ft>::temp_type __temp;
    typename lhg::arg_set<MaaGetImageWidth__ft>::call_type __call;
    if (!lhg::perform_input<MaaGetImageWidth__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaGetImageWidth__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaGetImageWidth, __call);
// LHG SEC BEGIN lhg.impl.MaaGetImageWidth.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaGetImageWidth__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaGetImageHeight.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaGetImageHeight_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaGetImageHeight__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaGetImageHeight__ft>::temp_type __temp;
    typename lhg::arg_set<MaaGetImageHeight__ft>::call_type __call;
    if (!lhg::perform_input<MaaGetImageHeight__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaGetImageHeight__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaGetImageHeight, __call);
// LHG SEC BEGIN lhg.impl.MaaGetImageHeight.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaGetImageHeight__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaGetImageType.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaGetImageType_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaGetImageType__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaGetImageType__ft>::temp_type __temp;
    typename lhg::arg_set<MaaGetImageType__ft>::call_type __call;
    if (!lhg::perform_input<MaaGetImageType__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaGetImageType__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaGetImageType, __call);
// LHG SEC BEGIN lhg.impl.MaaGetImageType.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaGetImageType__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaGetImageEncoded.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaGetImageEncoded_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaGetImageEncoded__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaGetImageEncoded__ft>::temp_type __temp;
    typename lhg::arg_set<MaaGetImageEncoded__ft>::call_type __call;
    if (!lhg::perform_input<MaaGetImageEncoded__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaGetImageEncoded__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaGetImageEncoded, __call);
// LHG SEC BEGIN lhg.impl.MaaGetImageEncoded.return
    std::string __ret(base64::to_base64(std::string_view(reinterpret_cast<char*>(__return), MaaGetImageEncodedSize(std::get<0>(__call)))));
// LHG SEC END
    return lhg::perform_output<MaaGetImageEncoded__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaSetImageEncoded.func
template<>
inline bool lhg::perform_check_entry<MaaSetImageEncoded__ft::data__at>(json::object& __param, std::string& __error) {
    if (!lhg::check_var<const char *>(__param["data"])) {
        __error = "data should be string";
        return false;
    }
    return true;
}

template<>
inline bool lhg::perform_check_entry<MaaSetImageEncoded__ft::size__at>(json::object& __param, std::string& __error) {
    return true;
}

template<>
struct lhg::temp_arg_t<MaaSetImageEncoded__ft::data__at> {
    using type = std::string;
};

template<>
struct lhg::temp_arg_t<MaaSetImageEncoded__ft::size__at> {
    using type = std::monostate;
};

template<>
inline bool lhg::perform_input_entry<MaaSetImageEncoded__ft, MaaSetImageEncoded__ft::data__at>(typename lhg::arg_set<MaaSetImageEncoded__ft>::temp_type& temp_data, json::object __param, std::string& __error) {
    std::get<MaaSetImageEncoded__ft::data__at::index>(temp_data) = base64::from_base64(__param[MaaSetImageEncoded__ft::data__at::name].as_string());
    return true;
}

template<>
inline bool lhg::perform_input_entry<MaaSetImageEncoded__ft, MaaSetImageEncoded__ft::size__at>(typename lhg::arg_set<MaaSetImageEncoded__ft>::temp_type& temp_data, json::object __param, std::string& __error) {
    return true;
}

template<>
inline bool lhg::perform_input_fix_entry<MaaSetImageEncoded__ft, MaaSetImageEncoded__ft::data__at>(typename lhg::arg_set<MaaSetImageEncoded__ft>::call_type& data, typename lhg::arg_set<MaaSetImageEncoded__ft>::temp_type& temp_data, json::object __param, std::string& __error) {
    const auto &str = std::get<MaaSetImageEncoded__ft::data__at::index>(temp_data);
    std::get<MaaSetImageEncoded__ft::data__at::index>(data) = reinterpret_cast<uint8_t *>(const_cast<char *>(str.c_str()));
    return true;
}

template<>
inline bool lhg::perform_input_fix_entry<MaaSetImageEncoded__ft, MaaSetImageEncoded__ft::size__at>(typename lhg::arg_set<MaaSetImageEncoded__ft>::call_type& data, typename lhg::arg_set<MaaSetImageEncoded__ft>::temp_type& temp_data, json::object __param, std::string& __error) {
    const auto &str = std::get<MaaSetImageEncoded__ft::data__at::index>(temp_data);
    std::get<MaaSetImageEncoded__ft::size__at::index>(data) = str.size();
    return true;
}
// LHG SEC END
std::optional<json::object> MaaSetImageEncoded_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaSetImageEncoded__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaSetImageEncoded__ft>::temp_type __temp;
    typename lhg::arg_set<MaaSetImageEncoded__ft>::call_type __call;
    if (!lhg::perform_input<MaaSetImageEncoded__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaSetImageEncoded__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaSetImageEncoded, __call);
// LHG SEC BEGIN lhg.impl.MaaSetImageEncoded.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaSetImageEncoded__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaVersion.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaVersion_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaVersion__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaVersion__ft>::temp_type __temp;
    typename lhg::arg_set<MaaVersion__ft>::call_type __call;
    if (!lhg::perform_input<MaaVersion__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaVersion__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaVersion();
// LHG SEC BEGIN lhg.impl.MaaVersion.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaVersion__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitInit.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitInit_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitInit__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitInit__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitInit__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitInit__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitInit__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitInit();
// LHG SEC BEGIN lhg.impl.MaaToolkitInit.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitInit__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitUninit.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitUninit_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitUninit__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitUninit__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitUninit__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitUninit__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitUninit__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitUninit();
// LHG SEC BEGIN lhg.impl.MaaToolkitUninit.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitUninit__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitFindDevice.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitFindDevice_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitFindDevice__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitFindDevice__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitFindDevice__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitFindDevice__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitFindDevice__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitFindDevice();
// LHG SEC BEGIN lhg.impl.MaaToolkitFindDevice.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitFindDevice__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitFindDeviceWithAdb.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitFindDeviceWithAdb_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitFindDeviceWithAdb__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitFindDeviceWithAdb__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitFindDeviceWithAdb__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitFindDeviceWithAdb__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitFindDeviceWithAdb__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitFindDeviceWithAdb, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitFindDeviceWithAdb.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitFindDeviceWithAdb__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitPostFindDevice.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitPostFindDevice_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitPostFindDevice__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitPostFindDevice__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitPostFindDevice__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitPostFindDevice__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitPostFindDevice__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitPostFindDevice();
// LHG SEC BEGIN lhg.impl.MaaToolkitPostFindDevice.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitPostFindDevice__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitPostFindDeviceWithAdb.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitPostFindDeviceWithAdb_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitPostFindDeviceWithAdb__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitPostFindDeviceWithAdb__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitPostFindDeviceWithAdb__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitPostFindDeviceWithAdb__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitPostFindDeviceWithAdb__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitPostFindDeviceWithAdb, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitPostFindDeviceWithAdb.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitPostFindDeviceWithAdb__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitIsFindDeviceCompleted.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitIsFindDeviceCompleted_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitIsFindDeviceCompleted__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitIsFindDeviceCompleted__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitIsFindDeviceCompleted__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitIsFindDeviceCompleted__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitIsFindDeviceCompleted__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitIsFindDeviceCompleted();
// LHG SEC BEGIN lhg.impl.MaaToolkitIsFindDeviceCompleted.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitIsFindDeviceCompleted__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitWaitForFindDeviceToComplete.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitWaitForFindDeviceToComplete_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitWaitForFindDeviceToComplete__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitWaitForFindDeviceToComplete__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitWaitForFindDeviceToComplete__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitWaitForFindDeviceToComplete__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitWaitForFindDeviceToComplete__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitWaitForFindDeviceToComplete();
// LHG SEC BEGIN lhg.impl.MaaToolkitWaitForFindDeviceToComplete.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitWaitForFindDeviceToComplete__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceCount.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetDeviceCount_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitGetDeviceCount__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitGetDeviceCount__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitGetDeviceCount__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitGetDeviceCount__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitGetDeviceCount__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitGetDeviceCount();
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceCount.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitGetDeviceCount__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceName.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetDeviceName_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitGetDeviceName__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitGetDeviceName__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitGetDeviceName__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitGetDeviceName__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitGetDeviceName__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitGetDeviceName, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceName.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitGetDeviceName__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceAdbPath.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetDeviceAdbPath_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitGetDeviceAdbPath__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitGetDeviceAdbPath__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitGetDeviceAdbPath__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitGetDeviceAdbPath__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitGetDeviceAdbPath__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitGetDeviceAdbPath, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbPath.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitGetDeviceAdbPath__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceAdbSerial.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetDeviceAdbSerial_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitGetDeviceAdbSerial__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitGetDeviceAdbSerial__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitGetDeviceAdbSerial__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitGetDeviceAdbSerial__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitGetDeviceAdbSerial__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitGetDeviceAdbSerial, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbSerial.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitGetDeviceAdbSerial__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceAdbControllerType.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetDeviceAdbControllerType_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitGetDeviceAdbControllerType__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitGetDeviceAdbControllerType__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitGetDeviceAdbControllerType__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitGetDeviceAdbControllerType__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitGetDeviceAdbControllerType__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitGetDeviceAdbControllerType, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbControllerType.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitGetDeviceAdbControllerType__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetDeviceAdbConfig.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetDeviceAdbConfig_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitGetDeviceAdbConfig__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitGetDeviceAdbConfig__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitGetDeviceAdbConfig__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitGetDeviceAdbConfig__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitGetDeviceAdbConfig__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitGetDeviceAdbConfig, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDeviceAdbConfig.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitGetDeviceAdbConfig__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitRegisterCustomRecognizerExecutor.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitRegisterCustomRecognizerExecutor_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitRegisterCustomRecognizerExecutor__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitRegisterCustomRecognizerExecutor__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitRegisterCustomRecognizerExecutor__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitRegisterCustomRecognizerExecutor__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitRegisterCustomRecognizerExecutor__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitRegisterCustomRecognizerExecutor, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomRecognizerExecutor.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitRegisterCustomRecognizerExecutor__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitUnregisterCustomRecognizerExecutor.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitUnregisterCustomRecognizerExecutor_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitUnregisterCustomRecognizerExecutor__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitUnregisterCustomRecognizerExecutor__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitUnregisterCustomRecognizerExecutor__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitUnregisterCustomRecognizerExecutor__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitUnregisterCustomRecognizerExecutor__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitUnregisterCustomRecognizerExecutor, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitUnregisterCustomRecognizerExecutor.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitUnregisterCustomRecognizerExecutor__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitRegisterCustomActionExecutor.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitRegisterCustomActionExecutor_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitRegisterCustomActionExecutor__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitRegisterCustomActionExecutor__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitRegisterCustomActionExecutor__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitRegisterCustomActionExecutor__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitRegisterCustomActionExecutor__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitRegisterCustomActionExecutor, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitRegisterCustomActionExecutor.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitRegisterCustomActionExecutor__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitUnregisterCustomActionExecutor.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitUnregisterCustomActionExecutor_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitUnregisterCustomActionExecutor__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitUnregisterCustomActionExecutor__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitUnregisterCustomActionExecutor__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitUnregisterCustomActionExecutor__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitUnregisterCustomActionExecutor__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitUnregisterCustomActionExecutor, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitUnregisterCustomActionExecutor.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitUnregisterCustomActionExecutor__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitFindWindow.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitFindWindow_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitFindWindow__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitFindWindow__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitFindWindow__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitFindWindow__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitFindWindow__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitFindWindow, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitFindWindow.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitFindWindow__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitSearchWindow.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitSearchWindow_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitSearchWindow__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitSearchWindow__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitSearchWindow__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitSearchWindow__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitSearchWindow__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitSearchWindow, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitSearchWindow.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitSearchWindow__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetWindow.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetWindow_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitGetWindow__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitGetWindow__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitGetWindow__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitGetWindow__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitGetWindow__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaToolkitGetWindow, __call);
// LHG SEC BEGIN lhg.impl.MaaToolkitGetWindow.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitGetWindow__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetCursorWindow.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetCursorWindow_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitGetCursorWindow__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitGetCursorWindow__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitGetCursorWindow__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitGetCursorWindow__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitGetCursorWindow__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitGetCursorWindow();
// LHG SEC BEGIN lhg.impl.MaaToolkitGetCursorWindow.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitGetCursorWindow__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetDesktopWindow.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetDesktopWindow_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitGetDesktopWindow__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitGetDesktopWindow__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitGetDesktopWindow__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitGetDesktopWindow__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitGetDesktopWindow__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitGetDesktopWindow();
// LHG SEC BEGIN lhg.impl.MaaToolkitGetDesktopWindow.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitGetDesktopWindow__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaToolkitGetForegroundWindow.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaToolkitGetForegroundWindow_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaToolkitGetForegroundWindow__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaToolkitGetForegroundWindow__ft>::temp_type __temp;
    typename lhg::arg_set<MaaToolkitGetForegroundWindow__ft>::call_type __call;
    if (!lhg::perform_input<MaaToolkitGetForegroundWindow__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaToolkitGetForegroundWindow__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = MaaToolkitGetForegroundWindow();
// LHG SEC BEGIN lhg.impl.MaaToolkitGetForegroundWindow.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaToolkitGetForegroundWindow__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerSetOptionString.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerSetOptionString_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerSetOptionString__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerSetOptionString__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerSetOptionString__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerSetOptionString__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerSetOptionString__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerSetOptionString, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerSetOptionString.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerSetOptionString__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerSetOptionInteger.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerSetOptionInteger_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerSetOptionInteger__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerSetOptionInteger__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerSetOptionInteger__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerSetOptionInteger__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerSetOptionInteger__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerSetOptionInteger, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerSetOptionInteger.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerSetOptionInteger__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaControllerSetOptionBoolean.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaControllerSetOptionBoolean_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaControllerSetOptionBoolean__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaControllerSetOptionBoolean__ft>::temp_type __temp;
    typename lhg::arg_set<MaaControllerSetOptionBoolean__ft>::call_type __call;
    if (!lhg::perform_input<MaaControllerSetOptionBoolean__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaControllerSetOptionBoolean__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaControllerSetOptionBoolean, __call);
// LHG SEC BEGIN lhg.impl.MaaControllerSetOptionBoolean.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaControllerSetOptionBoolean__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaSetGlobalOptionString.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaSetGlobalOptionString_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaSetGlobalOptionString__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaSetGlobalOptionString__ft>::temp_type __temp;
    typename lhg::arg_set<MaaSetGlobalOptionString__ft>::call_type __call;
    if (!lhg::perform_input<MaaSetGlobalOptionString__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaSetGlobalOptionString__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaSetGlobalOptionString, __call);
// LHG SEC BEGIN lhg.impl.MaaSetGlobalOptionString.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaSetGlobalOptionString__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaSetGlobalOptionInteger.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaSetGlobalOptionInteger_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaSetGlobalOptionInteger__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaSetGlobalOptionInteger__ft>::temp_type __temp;
    typename lhg::arg_set<MaaSetGlobalOptionInteger__ft>::call_type __call;
    if (!lhg::perform_input<MaaSetGlobalOptionInteger__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaSetGlobalOptionInteger__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaSetGlobalOptionInteger, __call);
// LHG SEC BEGIN lhg.impl.MaaSetGlobalOptionInteger.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaSetGlobalOptionInteger__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaSetGlobalOptionBoolean.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaSetGlobalOptionBoolean_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaSetGlobalOptionBoolean__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaSetGlobalOptionBoolean__ft>::temp_type __temp;
    typename lhg::arg_set<MaaSetGlobalOptionBoolean__ft>::call_type __call;
    if (!lhg::perform_input<MaaSetGlobalOptionBoolean__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaSetGlobalOptionBoolean__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaSetGlobalOptionBoolean, __call);
// LHG SEC BEGIN lhg.impl.MaaSetGlobalOptionBoolean.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaSetGlobalOptionBoolean__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaRegisterCustomRecognizerImpl.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaRegisterCustomRecognizerImpl_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaRegisterCustomRecognizerImpl__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaRegisterCustomRecognizerImpl__ft>::temp_type __temp;
    typename lhg::arg_set<MaaRegisterCustomRecognizerImpl__ft>::call_type __call;
    if (!lhg::perform_input<MaaRegisterCustomRecognizerImpl__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaRegisterCustomRecognizerImpl__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaRegisterCustomRecognizerImpl, __call);
// LHG SEC BEGIN lhg.impl.MaaRegisterCustomRecognizerImpl.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaRegisterCustomRecognizerImpl__ft>(__call, __ret);
}

// LHG SEC BEGIN lhg.custom.MaaRegisterCustomActionImpl.func
// LHG SEC DEF

// LHG SEC END
std::optional<json::object> MaaRegisterCustomActionImpl_Wrapper(json::object __param, std::string &__error) {
    if (!lhg::perform_check<MaaRegisterCustomActionImpl__ft>(__param, __error)) {
        return std::nullopt;
    }

    typename lhg::arg_set<MaaRegisterCustomActionImpl__ft>::temp_type __temp;
    typename lhg::arg_set<MaaRegisterCustomActionImpl__ft>::call_type __call;
    if (!lhg::perform_input<MaaRegisterCustomActionImpl__ft>(__temp, __param, __error)) {
        return std::nullopt;
    }
    if (!lhg::perform_input_fix<MaaRegisterCustomActionImpl__ft>(__call, __temp, __param, __error)) {
        return std::nullopt;
    }

    auto __return = std::apply(MaaRegisterCustomActionImpl, __call);
// LHG SEC BEGIN lhg.impl.MaaRegisterCustomActionImpl.return
// LHG SEC DEF
    auto __ret = __return;
// LHG SEC END
    return lhg::perform_output<MaaRegisterCustomActionImpl__ft>(__call, __ret);
}

bool handle_request(Context& ctx, UrlSegments segs) {
    auto obj = json::parse(ctx.req_.body()).value_or(json::object {}).as_object();

    // callback MaaAPICallback
    if (lhg::handle_callback("MaaAPICallback", MaaAPICallback__Manager, ctx, segs, obj, [](const auto& args) {
// LHG SEC BEGIN lhg.cb.MaaAPICallback.0
// LHG SEC DEF
        auto v0 = std::get<0>(args);
// LHG SEC END
// LHG SEC BEGIN lhg.cb.MaaAPICallback.1
// LHG SEC DEF
        auto v1 = std::get<1>(args);
// LHG SEC END
        return json::object {
            { "msg", v0 },
            { "details_json", v1 },
        };
    }, [](const auto& ret) {
        return 0;
    }, [](const auto& arg, const auto& res) {
    })) {
        return true;
    }

    // callback CustomActionRun
    if (lhg::handle_callback("CustomActionRun", CustomActionRun__Manager, ctx, segs, obj, [](const auto& args) {
// LHG SEC BEGIN lhg.cb.CustomActionRun.0
        // auto v0 = std::get<0>(args);
        std::string v0 = "sync ctx not impl";
// LHG SEC END
// LHG SEC BEGIN lhg.cb.CustomActionRun.1
// LHG SEC DEF
        auto v1 = std::get<1>(args);
// LHG SEC END
// LHG SEC BEGIN lhg.cb.CustomActionRun.2
// LHG SEC DEF
        auto v2 = std::get<2>(args);
// LHG SEC END
// LHG SEC BEGIN lhg.cb.CustomActionRun.3
        MaaRectHandle v3_ = std::get<3>(args);
        auto v3 = json::object {
            {"x", v3_->x},
            {"y", v3_->y},
            {"width", v3_->width},
            {"height", v3_->height}
        };
// LHG SEC END
// LHG SEC BEGIN lhg.cb.CustomActionRun.4
// LHG SEC DEF
        auto v4 = std::get<4>(args);
// LHG SEC END
        return json::object {
            { "sync_context", v0 },
            { "task_name", v1 },
            { "custom_action_param", v2 },
            { "cur_box", v3 },
            { "cur_rec_detail", v4 },
        };
    }, [](const auto& ret) -> std::optional<unsigned char> {
        if constexpr (lhg::check_t<unsigned char>::enable) {
            if (!lhg::check_var<unsigned char>(ret)) {
                return std::nullopt;
            }
        }
        return lhg::from_json<unsigned char>(ret);
    }, [](const auto& arg, const auto& res) {
    })) {
        return true;
    }

    // callback CustomActionStop
    if (lhg::handle_callback("CustomActionStop", CustomActionStop__Manager, ctx, segs, obj, [](const auto& args) {
        return json::object {

        };
    }, [](const auto& ret) {
        return 0;
    }, [](const auto& arg, const auto& res) {
    })) {
        return true;
    }

    // callback CustomRecognizerAnalyze
    if (lhg::handle_callback("CustomRecognizerAnalyze", CustomRecognizerAnalyze__Manager, ctx, segs, obj, [](const auto& args) {
// LHG SEC BEGIN lhg.cb.CustomRecognizerAnalyze.0
// LHG SEC DEF
        auto v0 = std::get<0>(args);
// LHG SEC END
// LHG SEC BEGIN lhg.cb.CustomRecognizerAnalyze.1
// LHG SEC DEF
        auto v1 = std::get<1>(args);
// LHG SEC END
// LHG SEC BEGIN lhg.cb.CustomRecognizerAnalyze.2
// LHG SEC DEF
        auto v2 = std::get<2>(args);
// LHG SEC END
// LHG SEC BEGIN lhg.cb.CustomRecognizerAnalyze.3
// LHG SEC DEF
        auto v3 = std::get<3>(args);
// LHG SEC END
        return json::object {
            { "sync_context", v0 },
            { "image", v1 },
            { "task_name", v2 },
            { "custom_recognition_param", v3 },
        };
    }, [](const auto& ret) -> std::optional<unsigned char> {
        if constexpr (lhg::check_t<unsigned char>::enable) {
            if (!lhg::check_var<unsigned char>(ret)) {
                return std::nullopt;
            }
        }
        return lhg::from_json<unsigned char>(ret);
    }, [](const auto& arg, const auto& res) {
        auto _out_box = res.find("out_box");
        auto v_out_box = std::get<5>(arg);
// LHG SEC BEGIN lhg.callback.output.fix.5
        auto out_box_obj = _out_box->as_object();
        v_out_box->x = out_box_obj.find("x")->as_integer();
        v_out_box->y = out_box_obj.find("y")->as_integer();
        v_out_box->width = out_box_obj.find("width")->as_integer();
        v_out_box->height = out_box_obj.find("height")->as_integer();
// LHG SEC END
        auto _out_detail = res.find("out_detail");
        auto v_out_detail = std::get<6>(arg);
// LHG SEC BEGIN lhg.callback.output.fix.6
        MaaSetString(v_out_detail, _out_detail->as_string().c_str());
// LHG SEC END
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

    // opaque MaaSyncContextAPI
    if (lhg::handle_opaque("MaaSyncContextAPI", MaaSyncContextAPI__OpaqueManager, ctx, segs, obj)) {
        return true;
    }

    const static lhg::api_info_map wrappers = {
        { "MaaAdbControllerCreate", { &MaaAdbControllerCreate_Wrapper, &lhg::input_helper<MaaAdbControllerCreate__ft>, &lhg::output_helper<MaaAdbControllerCreate__ft> } },
        { "MaaAdbControllerCreateV2", { &MaaAdbControllerCreateV2_Wrapper, &lhg::input_helper<MaaAdbControllerCreateV2__ft>, &lhg::output_helper<MaaAdbControllerCreateV2__ft> } },
        { "MaaThriftControllerCreate", { &MaaThriftControllerCreate_Wrapper, &lhg::input_helper<MaaThriftControllerCreate__ft>, &lhg::output_helper<MaaThriftControllerCreate__ft> } },
        { "MaaDbgControllerCreate", { &MaaDbgControllerCreate_Wrapper, &lhg::input_helper<MaaDbgControllerCreate__ft>, &lhg::output_helper<MaaDbgControllerCreate__ft> } },
        { "MaaControllerDestroy", { &MaaControllerDestroy_Wrapper, &lhg::input_helper<MaaControllerDestroy__ft>, &lhg::output_helper<MaaControllerDestroy__ft> } },
        { "MaaControllerPostConnection", { &MaaControllerPostConnection_Wrapper, &lhg::input_helper<MaaControllerPostConnection__ft>, &lhg::output_helper<MaaControllerPostConnection__ft> } },
        { "MaaControllerPostClick", { &MaaControllerPostClick_Wrapper, &lhg::input_helper<MaaControllerPostClick__ft>, &lhg::output_helper<MaaControllerPostClick__ft> } },
        { "MaaControllerPostSwipe", { &MaaControllerPostSwipe_Wrapper, &lhg::input_helper<MaaControllerPostSwipe__ft>, &lhg::output_helper<MaaControllerPostSwipe__ft> } },
        { "MaaControllerPostPressKey", { &MaaControllerPostPressKey_Wrapper, &lhg::input_helper<MaaControllerPostPressKey__ft>, &lhg::output_helper<MaaControllerPostPressKey__ft> } },
        { "MaaControllerPostInputText", { &MaaControllerPostInputText_Wrapper, &lhg::input_helper<MaaControllerPostInputText__ft>, &lhg::output_helper<MaaControllerPostInputText__ft> } },
        { "MaaControllerPostTouchDown", { &MaaControllerPostTouchDown_Wrapper, &lhg::input_helper<MaaControllerPostTouchDown__ft>, &lhg::output_helper<MaaControllerPostTouchDown__ft> } },
        { "MaaControllerPostTouchMove", { &MaaControllerPostTouchMove_Wrapper, &lhg::input_helper<MaaControllerPostTouchMove__ft>, &lhg::output_helper<MaaControllerPostTouchMove__ft> } },
        { "MaaControllerPostTouchUp", { &MaaControllerPostTouchUp_Wrapper, &lhg::input_helper<MaaControllerPostTouchUp__ft>, &lhg::output_helper<MaaControllerPostTouchUp__ft> } },
        { "MaaControllerPostScreencap", { &MaaControllerPostScreencap_Wrapper, &lhg::input_helper<MaaControllerPostScreencap__ft>, &lhg::output_helper<MaaControllerPostScreencap__ft> } },
        { "MaaControllerStatus", { &MaaControllerStatus_Wrapper, &lhg::input_helper<MaaControllerStatus__ft>, &lhg::output_helper<MaaControllerStatus__ft> } },
        { "MaaControllerWait", { &MaaControllerWait_Wrapper, &lhg::input_helper<MaaControllerWait__ft>, &lhg::output_helper<MaaControllerWait__ft> } },
        { "MaaControllerConnected", { &MaaControllerConnected_Wrapper, &lhg::input_helper<MaaControllerConnected__ft>, &lhg::output_helper<MaaControllerConnected__ft> } },
        { "MaaControllerGetImage", { &MaaControllerGetImage_Wrapper, &lhg::input_helper<MaaControllerGetImage__ft>, &lhg::output_helper<MaaControllerGetImage__ft> } },
        { "MaaControllerGetUUID", { &MaaControllerGetUUID_Wrapper, &lhg::input_helper<MaaControllerGetUUID__ft>, &lhg::output_helper<MaaControllerGetUUID__ft> } },
        { "MaaCreate", { &MaaCreate_Wrapper, &lhg::input_helper<MaaCreate__ft>, &lhg::output_helper<MaaCreate__ft> } },
        { "MaaDestroy", { &MaaDestroy_Wrapper, &lhg::input_helper<MaaDestroy__ft>, &lhg::output_helper<MaaDestroy__ft> } },
        { "MaaBindResource", { &MaaBindResource_Wrapper, &lhg::input_helper<MaaBindResource__ft>, &lhg::output_helper<MaaBindResource__ft> } },
        { "MaaBindController", { &MaaBindController_Wrapper, &lhg::input_helper<MaaBindController__ft>, &lhg::output_helper<MaaBindController__ft> } },
        { "MaaInited", { &MaaInited_Wrapper, &lhg::input_helper<MaaInited__ft>, &lhg::output_helper<MaaInited__ft> } },
        { "MaaUnregisterCustomRecognizer", { &MaaUnregisterCustomRecognizer_Wrapper, &lhg::input_helper<MaaUnregisterCustomRecognizer__ft>, &lhg::output_helper<MaaUnregisterCustomRecognizer__ft> } },
        { "MaaClearCustomRecognizer", { &MaaClearCustomRecognizer_Wrapper, &lhg::input_helper<MaaClearCustomRecognizer__ft>, &lhg::output_helper<MaaClearCustomRecognizer__ft> } },
        { "MaaUnregisterCustomAction", { &MaaUnregisterCustomAction_Wrapper, &lhg::input_helper<MaaUnregisterCustomAction__ft>, &lhg::output_helper<MaaUnregisterCustomAction__ft> } },
        { "MaaClearCustomAction", { &MaaClearCustomAction_Wrapper, &lhg::input_helper<MaaClearCustomAction__ft>, &lhg::output_helper<MaaClearCustomAction__ft> } },
        { "MaaPostTask", { &MaaPostTask_Wrapper, &lhg::input_helper<MaaPostTask__ft>, &lhg::output_helper<MaaPostTask__ft> } },
        { "MaaSetTaskParam", { &MaaSetTaskParam_Wrapper, &lhg::input_helper<MaaSetTaskParam__ft>, &lhg::output_helper<MaaSetTaskParam__ft> } },
        { "MaaTaskStatus", { &MaaTaskStatus_Wrapper, &lhg::input_helper<MaaTaskStatus__ft>, &lhg::output_helper<MaaTaskStatus__ft> } },
        { "MaaWaitTask", { &MaaWaitTask_Wrapper, &lhg::input_helper<MaaWaitTask__ft>, &lhg::output_helper<MaaWaitTask__ft> } },
        { "MaaTaskAllFinished", { &MaaTaskAllFinished_Wrapper, &lhg::input_helper<MaaTaskAllFinished__ft>, &lhg::output_helper<MaaTaskAllFinished__ft> } },
        { "MaaPostStop", { &MaaPostStop_Wrapper, &lhg::input_helper<MaaPostStop__ft>, &lhg::output_helper<MaaPostStop__ft> } },
        { "MaaStop", { &MaaStop_Wrapper, &lhg::input_helper<MaaStop__ft>, &lhg::output_helper<MaaStop__ft> } },
        { "MaaGetResource", { &MaaGetResource_Wrapper, &lhg::input_helper<MaaGetResource__ft>, &lhg::output_helper<MaaGetResource__ft> } },
        { "MaaGetController", { &MaaGetController_Wrapper, &lhg::input_helper<MaaGetController__ft>, &lhg::output_helper<MaaGetController__ft> } },
        { "MaaResourceCreate", { &MaaResourceCreate_Wrapper, &lhg::input_helper<MaaResourceCreate__ft>, &lhg::output_helper<MaaResourceCreate__ft> } },
        { "MaaResourceDestroy", { &MaaResourceDestroy_Wrapper, &lhg::input_helper<MaaResourceDestroy__ft>, &lhg::output_helper<MaaResourceDestroy__ft> } },
        { "MaaResourcePostPath", { &MaaResourcePostPath_Wrapper, &lhg::input_helper<MaaResourcePostPath__ft>, &lhg::output_helper<MaaResourcePostPath__ft> } },
        { "MaaResourceStatus", { &MaaResourceStatus_Wrapper, &lhg::input_helper<MaaResourceStatus__ft>, &lhg::output_helper<MaaResourceStatus__ft> } },
        { "MaaResourceWait", { &MaaResourceWait_Wrapper, &lhg::input_helper<MaaResourceWait__ft>, &lhg::output_helper<MaaResourceWait__ft> } },
        { "MaaResourceLoaded", { &MaaResourceLoaded_Wrapper, &lhg::input_helper<MaaResourceLoaded__ft>, &lhg::output_helper<MaaResourceLoaded__ft> } },
        { "MaaResourceGetHash", { &MaaResourceGetHash_Wrapper, &lhg::input_helper<MaaResourceGetHash__ft>, &lhg::output_helper<MaaResourceGetHash__ft> } },
        { "MaaResourceGetTaskList", { &MaaResourceGetTaskList_Wrapper, &lhg::input_helper<MaaResourceGetTaskList__ft>, &lhg::output_helper<MaaResourceGetTaskList__ft> } },
        { "MaaCreateImageBuffer", { &MaaCreateImageBuffer_Wrapper, &lhg::input_helper<MaaCreateImageBuffer__ft>, &lhg::output_helper<MaaCreateImageBuffer__ft> } },
        { "MaaDestroyImageBuffer", { &MaaDestroyImageBuffer_Wrapper, &lhg::input_helper<MaaDestroyImageBuffer__ft>, &lhg::output_helper<MaaDestroyImageBuffer__ft> } },
        { "MaaIsImageEmpty", { &MaaIsImageEmpty_Wrapper, &lhg::input_helper<MaaIsImageEmpty__ft>, &lhg::output_helper<MaaIsImageEmpty__ft> } },
        { "MaaClearImage", { &MaaClearImage_Wrapper, &lhg::input_helper<MaaClearImage__ft>, &lhg::output_helper<MaaClearImage__ft> } },
        { "MaaGetImageWidth", { &MaaGetImageWidth_Wrapper, &lhg::input_helper<MaaGetImageWidth__ft>, &lhg::output_helper<MaaGetImageWidth__ft> } },
        { "MaaGetImageHeight", { &MaaGetImageHeight_Wrapper, &lhg::input_helper<MaaGetImageHeight__ft>, &lhg::output_helper<MaaGetImageHeight__ft> } },
        { "MaaGetImageType", { &MaaGetImageType_Wrapper, &lhg::input_helper<MaaGetImageType__ft>, &lhg::output_helper<MaaGetImageType__ft> } },
        { "MaaGetImageEncoded", { &MaaGetImageEncoded_Wrapper, &lhg::input_helper<MaaGetImageEncoded__ft>, &lhg::output_helper<MaaGetImageEncoded__ft> } },
        { "MaaSetImageEncoded", { &MaaSetImageEncoded_Wrapper, &lhg::input_helper<MaaSetImageEncoded__ft>, &lhg::output_helper<MaaSetImageEncoded__ft> } },
        { "MaaVersion", { &MaaVersion_Wrapper, &lhg::input_helper<MaaVersion__ft>, &lhg::output_helper<MaaVersion__ft> } },
        { "MaaToolkitInit", { &MaaToolkitInit_Wrapper, &lhg::input_helper<MaaToolkitInit__ft>, &lhg::output_helper<MaaToolkitInit__ft> } },
        { "MaaToolkitUninit", { &MaaToolkitUninit_Wrapper, &lhg::input_helper<MaaToolkitUninit__ft>, &lhg::output_helper<MaaToolkitUninit__ft> } },
        { "MaaToolkitFindDevice", { &MaaToolkitFindDevice_Wrapper, &lhg::input_helper<MaaToolkitFindDevice__ft>, &lhg::output_helper<MaaToolkitFindDevice__ft> } },
        { "MaaToolkitFindDeviceWithAdb", { &MaaToolkitFindDeviceWithAdb_Wrapper, &lhg::input_helper<MaaToolkitFindDeviceWithAdb__ft>, &lhg::output_helper<MaaToolkitFindDeviceWithAdb__ft> } },
        { "MaaToolkitPostFindDevice", { &MaaToolkitPostFindDevice_Wrapper, &lhg::input_helper<MaaToolkitPostFindDevice__ft>, &lhg::output_helper<MaaToolkitPostFindDevice__ft> } },
        { "MaaToolkitPostFindDeviceWithAdb", { &MaaToolkitPostFindDeviceWithAdb_Wrapper, &lhg::input_helper<MaaToolkitPostFindDeviceWithAdb__ft>, &lhg::output_helper<MaaToolkitPostFindDeviceWithAdb__ft> } },
        { "MaaToolkitIsFindDeviceCompleted", { &MaaToolkitIsFindDeviceCompleted_Wrapper, &lhg::input_helper<MaaToolkitIsFindDeviceCompleted__ft>, &lhg::output_helper<MaaToolkitIsFindDeviceCompleted__ft> } },
        { "MaaToolkitWaitForFindDeviceToComplete", { &MaaToolkitWaitForFindDeviceToComplete_Wrapper, &lhg::input_helper<MaaToolkitWaitForFindDeviceToComplete__ft>, &lhg::output_helper<MaaToolkitWaitForFindDeviceToComplete__ft> } },
        { "MaaToolkitGetDeviceCount", { &MaaToolkitGetDeviceCount_Wrapper, &lhg::input_helper<MaaToolkitGetDeviceCount__ft>, &lhg::output_helper<MaaToolkitGetDeviceCount__ft> } },
        { "MaaToolkitGetDeviceName", { &MaaToolkitGetDeviceName_Wrapper, &lhg::input_helper<MaaToolkitGetDeviceName__ft>, &lhg::output_helper<MaaToolkitGetDeviceName__ft> } },
        { "MaaToolkitGetDeviceAdbPath", { &MaaToolkitGetDeviceAdbPath_Wrapper, &lhg::input_helper<MaaToolkitGetDeviceAdbPath__ft>, &lhg::output_helper<MaaToolkitGetDeviceAdbPath__ft> } },
        { "MaaToolkitGetDeviceAdbSerial", { &MaaToolkitGetDeviceAdbSerial_Wrapper, &lhg::input_helper<MaaToolkitGetDeviceAdbSerial__ft>, &lhg::output_helper<MaaToolkitGetDeviceAdbSerial__ft> } },
        { "MaaToolkitGetDeviceAdbControllerType", { &MaaToolkitGetDeviceAdbControllerType_Wrapper, &lhg::input_helper<MaaToolkitGetDeviceAdbControllerType__ft>, &lhg::output_helper<MaaToolkitGetDeviceAdbControllerType__ft> } },
        { "MaaToolkitGetDeviceAdbConfig", { &MaaToolkitGetDeviceAdbConfig_Wrapper, &lhg::input_helper<MaaToolkitGetDeviceAdbConfig__ft>, &lhg::output_helper<MaaToolkitGetDeviceAdbConfig__ft> } },
        { "MaaToolkitRegisterCustomRecognizerExecutor", { &MaaToolkitRegisterCustomRecognizerExecutor_Wrapper, &lhg::input_helper<MaaToolkitRegisterCustomRecognizerExecutor__ft>, &lhg::output_helper<MaaToolkitRegisterCustomRecognizerExecutor__ft> } },
        { "MaaToolkitUnregisterCustomRecognizerExecutor", { &MaaToolkitUnregisterCustomRecognizerExecutor_Wrapper, &lhg::input_helper<MaaToolkitUnregisterCustomRecognizerExecutor__ft>, &lhg::output_helper<MaaToolkitUnregisterCustomRecognizerExecutor__ft> } },
        { "MaaToolkitRegisterCustomActionExecutor", { &MaaToolkitRegisterCustomActionExecutor_Wrapper, &lhg::input_helper<MaaToolkitRegisterCustomActionExecutor__ft>, &lhg::output_helper<MaaToolkitRegisterCustomActionExecutor__ft> } },
        { "MaaToolkitUnregisterCustomActionExecutor", { &MaaToolkitUnregisterCustomActionExecutor_Wrapper, &lhg::input_helper<MaaToolkitUnregisterCustomActionExecutor__ft>, &lhg::output_helper<MaaToolkitUnregisterCustomActionExecutor__ft> } },
        { "MaaToolkitFindWindow", { &MaaToolkitFindWindow_Wrapper, &lhg::input_helper<MaaToolkitFindWindow__ft>, &lhg::output_helper<MaaToolkitFindWindow__ft> } },
        { "MaaToolkitSearchWindow", { &MaaToolkitSearchWindow_Wrapper, &lhg::input_helper<MaaToolkitSearchWindow__ft>, &lhg::output_helper<MaaToolkitSearchWindow__ft> } },
        { "MaaToolkitGetWindow", { &MaaToolkitGetWindow_Wrapper, &lhg::input_helper<MaaToolkitGetWindow__ft>, &lhg::output_helper<MaaToolkitGetWindow__ft> } },
        { "MaaToolkitGetCursorWindow", { &MaaToolkitGetCursorWindow_Wrapper, &lhg::input_helper<MaaToolkitGetCursorWindow__ft>, &lhg::output_helper<MaaToolkitGetCursorWindow__ft> } },
        { "MaaToolkitGetDesktopWindow", { &MaaToolkitGetDesktopWindow_Wrapper, &lhg::input_helper<MaaToolkitGetDesktopWindow__ft>, &lhg::output_helper<MaaToolkitGetDesktopWindow__ft> } },
        { "MaaToolkitGetForegroundWindow", { &MaaToolkitGetForegroundWindow_Wrapper, &lhg::input_helper<MaaToolkitGetForegroundWindow__ft>, &lhg::output_helper<MaaToolkitGetForegroundWindow__ft> } },
        { "MaaControllerSetOptionString", { &MaaControllerSetOptionString_Wrapper, &lhg::input_helper<MaaControllerSetOptionString__ft>, &lhg::output_helper<MaaControllerSetOptionString__ft> } },
        { "MaaControllerSetOptionInteger", { &MaaControllerSetOptionInteger_Wrapper, &lhg::input_helper<MaaControllerSetOptionInteger__ft>, &lhg::output_helper<MaaControllerSetOptionInteger__ft> } },
        { "MaaControllerSetOptionBoolean", { &MaaControllerSetOptionBoolean_Wrapper, &lhg::input_helper<MaaControllerSetOptionBoolean__ft>, &lhg::output_helper<MaaControllerSetOptionBoolean__ft> } },
        { "MaaSetGlobalOptionString", { &MaaSetGlobalOptionString_Wrapper, &lhg::input_helper<MaaSetGlobalOptionString__ft>, &lhg::output_helper<MaaSetGlobalOptionString__ft> } },
        { "MaaSetGlobalOptionInteger", { &MaaSetGlobalOptionInteger_Wrapper, &lhg::input_helper<MaaSetGlobalOptionInteger__ft>, &lhg::output_helper<MaaSetGlobalOptionInteger__ft> } },
        { "MaaSetGlobalOptionBoolean", { &MaaSetGlobalOptionBoolean_Wrapper, &lhg::input_helper<MaaSetGlobalOptionBoolean__ft>, &lhg::output_helper<MaaSetGlobalOptionBoolean__ft> } },
        { "MaaRegisterCustomRecognizerImpl", { &MaaRegisterCustomRecognizerImpl_Wrapper, &lhg::input_helper<MaaRegisterCustomRecognizerImpl__ft>, &lhg::output_helper<MaaRegisterCustomRecognizerImpl__ft> } },
        { "MaaRegisterCustomActionImpl", { &MaaRegisterCustomActionImpl_Wrapper, &lhg::input_helper<MaaRegisterCustomActionImpl__ft>, &lhg::output_helper<MaaRegisterCustomActionImpl__ft> } },
    };
    if (lhg::handle_api(ctx, segs, obj, wrappers)) {
        return true;
    }

    if (lhg::handle_help(ctx, segs, wrappers, { "MaaControllerAPI", "MaaResourceAPI", "MaaInstanceAPI", "MaaImageBuffer", "MaaSyncContextAPI" }, [](json::object& result) {
        lhg::help_callback("MaaAPICallback", result, json::object { { "msg", json::parse(lhg::schema_t<decltype(std::get<0>(lhg::callback_manager<void (*)(const char *, const char *, void *)>::CallbackContext::args_type {}))>::schema).value() }, { "details_json", json::parse(lhg::schema_t<decltype(std::get<1>(lhg::callback_manager<void (*)(const char *, const char *, void *)>::CallbackContext::args_type {}))>::schema).value() } },
        std::nullopt,
        json::object {

        });

        lhg::help_callback("CustomActionRun", result, json::object { { "sync_context", json::parse(lhg::schema_t<decltype(std::get<0>(lhg::callback_manager<unsigned char (*)(MaaSyncContextAPI *, const char *, const char *, MaaRect *, const char *, void *)>::CallbackContext::args_type {}))>::schema).value() }, { "task_name", json::parse(lhg::schema_t<decltype(std::get<1>(lhg::callback_manager<unsigned char (*)(MaaSyncContextAPI *, const char *, const char *, MaaRect *, const char *, void *)>::CallbackContext::args_type {}))>::schema).value() }, { "custom_action_param", json::parse(lhg::schema_t<decltype(std::get<2>(lhg::callback_manager<unsigned char (*)(MaaSyncContextAPI *, const char *, const char *, MaaRect *, const char *, void *)>::CallbackContext::args_type {}))>::schema).value() }, { "cur_box", json::parse(lhg::schema_t<decltype(std::get<3>(lhg::callback_manager<unsigned char (*)(MaaSyncContextAPI *, const char *, const char *, MaaRect *, const char *, void *)>::CallbackContext::args_type {}))>::schema).value() }, { "cur_rec_detail", json::parse(lhg::schema_t<decltype(std::get<4>(lhg::callback_manager<unsigned char (*)(MaaSyncContextAPI *, const char *, const char *, MaaRect *, const char *, void *)>::CallbackContext::args_type {}))>::schema).value() } },
        json::parse(lhg::schema_t<unsigned char>::schema).value().as_object(),
        json::object {

        });

        lhg::help_callback("CustomActionStop", result, json::object {},
        std::nullopt,
        json::object {

        });

        lhg::help_callback("CustomRecognizerAnalyze", result, json::object { { "sync_context", json::parse(lhg::schema_t<decltype(std::get<0>(lhg::callback_manager<unsigned char (*)(MaaSyncContextAPI *, MaaImageBuffer *, const char *, const char *, void *, MaaRect *, MaaStringBuffer *)>::CallbackContext::args_type {}))>::schema).value() }, { "image", json::parse(lhg::schema_t<decltype(std::get<1>(lhg::callback_manager<unsigned char (*)(MaaSyncContextAPI *, MaaImageBuffer *, const char *, const char *, void *, MaaRect *, MaaStringBuffer *)>::CallbackContext::args_type {}))>::schema).value() }, { "task_name", json::parse(lhg::schema_t<decltype(std::get<2>(lhg::callback_manager<unsigned char (*)(MaaSyncContextAPI *, MaaImageBuffer *, const char *, const char *, void *, MaaRect *, MaaStringBuffer *)>::CallbackContext::args_type {}))>::schema).value() }, { "custom_recognition_param", json::parse(lhg::schema_t<decltype(std::get<3>(lhg::callback_manager<unsigned char (*)(MaaSyncContextAPI *, MaaImageBuffer *, const char *, const char *, void *, MaaRect *, MaaStringBuffer *)>::CallbackContext::args_type {}))>::schema).value() } },
        json::parse(lhg::schema_t<unsigned char>::schema).value().as_object(),
        json::object {
            { "out_box", json::parse(lhg::schema_t<decltype(std::get<5>(lhg::callback_manager<unsigned char (*)(MaaSyncContextAPI *, MaaImageBuffer *, const char *, const char *, void *, MaaRect *, MaaStringBuffer *)>::CallbackContext::args_type {}))>::schema).value() },
            { "out_detail", json::parse(lhg::schema_t<decltype(std::get<6>(lhg::callback_manager<unsigned char (*)(MaaSyncContextAPI *, MaaImageBuffer *, const char *, const char *, void *, MaaRect *, MaaStringBuffer *)>::CallbackContext::args_type {}))>::schema).value() },
        });
    })) {
        return true;
    }

    return false;
}
