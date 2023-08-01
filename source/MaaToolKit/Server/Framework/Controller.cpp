#include "Controller.h"
#include "../JsonValidator.hpp"
#include "MaaFramework/MaaAPI.h"
#include "Utils/Boost.hpp"

#include <map>

MAA_TOOLKIT_NS_BEGIN

auto generator = boost::uuids::random_generator();
std::map<boost::uuids::uuid, MaaControllerHandle> handles;

void controller_callback(MaaString msg, MaaJsonString details_json, MaaCallbackTransparentArg callback_arg)
{
    std::ignore = msg;
    std::ignore = details_json;
    std::ignore = callback_arg;
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
                auto config = require_key_as_string(obj, "config");
                if (!config.has_value()) {
                    return std::nullopt;
                }

                MaaAdbControllerType ntype = 0;
                for (const auto& t : type.value()) {
                    if (typeIdx.count(t)) {
                        ntype |= typeIdx[t];
                    }
                    else {
                        return std::nullopt;
                    }
                }

                return Param {
                    std::move(adb.value()),
                    std::move(address.value()),
                    ntype,
                    std::move(config.value()),
                };
            }
        };

        auto pparam = Param::from_json(req);
        if (!pparam.has_value()) {
            return std::nullopt;
        }
        const auto& param = pparam.value();

        boost::uuids::uuid id = generator();
        handles[id] = MaaAdbControllerCreate(param.adb.c_str(), param.address.c_str(), param.type, param.config.c_str(),
                                             controller_callback, 0);

        return json::object { { "uuid", boost::uuids::to_string(id) } };
    });
}

MAA_TOOLKIT_NS_END
