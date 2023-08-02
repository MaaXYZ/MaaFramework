#include "Controller.h"
#include "../JsonValidator.hpp"
#include "MaaFramework/MaaAPI.h"
#include "Utils/Boost.hpp"

#include <map>

MAA_TOOLKIT_SERVER_BEGIN

auto cfg = R"({
    "prebuilt": {
        "minicap": {
            "root": "./MaaAgentBinary/minicap",
            "arch": [
                "x86",
                "armeabi-v7a",
                "armeabi"
            ],
            "sdk": [
                31,
                29,
                28,
                27,
                26,
                25,
                24,
                23,
                22,
                21,
                19,
                18,
                17,
                16,
                15,
                14
            ]
        },
        "minitouch": {
            "root": "./MaaAgentBinary/minitouch",
            "arch": [
                "x86_64",
                "x86",
                "arm64-v8a",
                "armeabi-v7a",
                "armeabi"
            ]
        },
        "maatouch": {
            "root": "./MaaAgentBinary/maatouch",
            "package": "com.shxyke.MaaTouch.App"
        }
    },
    "argv": {
        "Connect": [
            "{ADB}",
            "connect",
            "{ADB_SERIAL}"
        ],
        "KillServer": [
            "{ADB}",
            "kill-server"
        ],
        "UUID": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "settings get secure android_id"
        ],
        "Resolution": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "dumpsys window displays | grep -o -E cur=+[^\\ ]+ | grep -o -E [0-9]+"
        ],
        "StartApp": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "am start -n {INTENT}"
        ],
        "StopApp": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "am force-stop {INTENT}"
        ],
        "Click": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "input tap {X} {Y}"
        ],
        "Swipe": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "input swipe {X1} {Y1} {X2} {Y2} {DURATION}"
        ],
        "PressKey": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "input keyevent {KEY}"
        ],
        "ForwardSocket": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "forward",
            "tcp:{FOWARD_PORT}",
            "localabstract:{LOCAL_SOCKET}"
        ],
        "NetcatAddress": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "cat /proc/net/arp | grep : "
        ],
        "ScreencapRawByNetcat": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "exec-out",
            "screencap | nc -w 3 {NETCAT_ADDRESS} {NETCAT_PORT}"
        ],
        "ScreencapRawWithGzip": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "exec-out",
            "screencap | gzip -1"
        ],
        "ScreencapEncode": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "exec-out",
            "screencap -p"
        ],
        "ScreencapEncodeToFile": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "screencap -p > \"/data/local/tmp/{TEMP_FILE}\""
        ],
        "PullFile": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "pull",
            "/data/local/tmp/{TEMP_FILE}",
            "{DST_PATH}"
        ],
        "Abilist": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "getprop ro.product.cpu.abilist | tr -d '\n\r'"
        ],
        "SDK": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "getprop ro.build.version.sdk | tr -d '\n\r'"
        ],
        "Orientation": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "dumpsys input | grep SurfaceOrientation | grep -m 1 -o -E [0-9]"
        ],
        "PushBin": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "push",
            "{BIN_PATH}",
            "/data/local/tmp/{BIN_WORKING_FILE}"
        ],
        "ChmodBin": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "chmod 700 \"/data/local/tmp/{BIN_WORKING_FILE}\""
        ],
        "InvokeBin": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "export LD_LIBRARY_PATH=/data/local/tmp/; \"/data/local/tmp/{BIN_WORKING_FILE}\" {BIN_EXTRA_PARAMS}"
        ],
        "InvokeApp": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "export CLASSPATH=\"/data/local/tmp/{APP_WORKING_FILE}\"; app_process /data/local/tmp {PACKAGE_NAME}"
        ]
    }
})";

struct ControllerInfo
{
    std::mutex lock;
    MaaControllerHandle handle;
    std::vector<json::object> callback;
};

auto generator = boost::uuids::random_generator();
auto from_str = boost::uuids::string_generator();
std::mutex handle_mtx;
std::map<boost::uuids::uuid, ControllerInfo*> handles;

void controller_callback(MaaString msg, MaaJsonString details_json, MaaCallbackTransparentArg callback_arg)
{
    ControllerInfo* info = reinterpret_cast<ControllerInfo*>(callback_arg);

    std::unique_lock<std::mutex> lock(info->lock);
    info->callback.emplace_back(json::object {
        { "msg", msg }, { "detail", json::parse(std::string_view(details_json)).value_or(json::object {}) } });
}

void init_maa_framework_controller(ApiDispatcher& disp)
{
    disp.register_route("controller.create.adb", [](json::object req) -> std::optional<json::object> {
        struct Param
        {
            std::string adb;
            std::string address;
            MaaAdbControllerType type;
            std::string config;

            static std::optional<Param> from_json(const json::object& obj)
            {
                static std::map<std::string, MaaAdbControllerType> typeIdx = {
                    { "touch.adb", MaaAdbControllerType_Touch_Adb },
                    { "touch.minitouch", MaaAdbControllerType_Touch_MiniTouch },
                    { "touch.maatouch", MaaAdbControllerType_Touch_MaaTouch },
                    { "key.adb", MaaAdbControllerType_Key_Adb },
                    { "key.maatouch", MaaAdbControllerType_Key_MaaTouch },
                    { "screencap.fastest", MaaAdbControllerType_Screencap_FastestWay },
                    { "screencap.rawbynetcat", MaaAdbControllerType_Screencap_RawByNetcat },
                    { "screencap.rawwithgzip", MaaAdbControllerType_Screencap_RawWithGzip },
                    { "screencap.encode", MaaAdbControllerType_Screencap_Encode },
                    { "screencap.encodetofile", MaaAdbControllerType_Screencap_EncodeToFile },
                    { "screencap.minicapdirect", MaaAdbControllerType_Screencap_MinicapDirect },
                    { "screencap.minicapstream", MaaAdbControllerType_Screencap_MinicapStream },
                };

                auto adb = require_key_as_string(obj, "adb");
                if (!adb.has_value()) {
                    return std::nullopt;
                }
                auto address = require_key_as_string(obj, "address");
                if (!address.has_value()) {
                    return std::nullopt;
                }
                auto type = require_key_as_string_array(obj, "type");
                if (!type.has_value()) {
                    return std::nullopt;
                }
                // auto config = require_key_as_string(obj, "config");
                // if (!config.has_value()) {
                //     return std::nullopt;
                // }

                MaaAdbControllerType ntype = 0;
                for (const auto& t : type.value()) {
                    if (typeIdx.count(t)) {
                        ntype |= typeIdx[t];
                    }
                    else {
                        return std::nullopt;
                    }
                }

                return Param { std::move(adb.value()), std::move(address.value()), ntype,
                               // std::move(config.value()),
                               cfg };
            }
        };

        auto pparam = Param::from_json(req);
        if (!pparam.has_value()) {
            return std::nullopt;
        }
        const auto& param = pparam.value();

        boost::uuids::uuid id = generator();
        auto ci = new ControllerInfo;
        ci->handle = MaaAdbControllerCreate(param.adb.c_str(), param.address.c_str(), param.type, param.config.c_str(),
                                            controller_callback, ci);
        {
            std::unique_lock<std::mutex> lock(handle_mtx);
            handles[id] = ci;
        }

        return json::object { { "uuid", boost::uuids::to_string(id) } };
    });

    disp.register_route("controller.destroy", [](json::object req) -> std::optional<json::object> {
        auto uuid = require_key_as_string(req, "uuid");
        if (!uuid.has_value()) {
            return std::nullopt;
        }
        auto id = from_str(uuid.value());
        ControllerInfo* ci;

        {
            std::unique_lock<std::mutex> lock(handle_mtx);
            if (handles.find(id) == handles.end()) {
                return std::nullopt;
            }
            ci = handles[id];
            handles.erase(handles.find(id));
            ci->lock.lock();
        }

        MaaControllerDestroy(ci->handle);
        delete ci;
        return json::object {};
    });

    // disp.register_route("controller.callback", [](json::object req) -> std::optional<json::object> {
    //     auto uuid = require_key_as_string(req, "uuid");
    //     if (!uuid.has_value()) {
    //         return std::nullopt;
    //     }
    //     auto id = from_str(uuid.value());
    //     ControllerInfo* ci;
    //     std::vector<json::object> arr;

    //     {
    //         std::unique_lock<std::mutex> lock(handle_mtx);
    //         if (handles.find(id) == handles.end()) {
    //             return std::nullopt;
    //         }
    //         ci = handles[id];
    //     }

    //     MaaControllerDestroy(ci->handle);
    //     delete ci;
    //     return json::object {};
    // });

    disp.register_route("controller.connect", [](json::object req) -> std::optional<json::object> {
        auto uuid = require_key_as_string(req, "uuid");
        if (!uuid.has_value()) {
            return std::nullopt;
        }
        auto id = from_str(uuid.value());
        std::unique_lock<std::mutex> lock(handle_mtx);
        if (handles.find(id) == handles.end()) {
            return std::nullopt;
        }
        return json::object { { "id", MaaControllerPostConnection(handles[id]->handle) } };
    });
}

MAA_TOOLKIT_SERVER_END
