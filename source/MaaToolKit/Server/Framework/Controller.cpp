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
    disp.register_route(ApiDispatcher::Method::post, "/api/controller/adb", [](RequestResponse& rr) {
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

        boost::uuids::uuid id = generator();

        auto obj = rr.request_body_json();
        auto adb = require_key_as_string(obj, "adb");
        auto address = require_key_as_string(obj, "address");
        auto type = require_key_as_string_array(obj, "type");
        auto config = require_key_as_string(obj, "config");

        MaaAdbControllerType ntype = 0;
        for (const auto& t : type) {
            if (typeIdx.count(t)) {
                ntype |= typeIdx[t];
            }
            else {
                throw JsonValidateFailedException("unknown type " + t);
            }
        }

        handles[id] =
            MaaAdbControllerCreate(adb.c_str(), address.c_str(), ntype, config.c_str(), controller_callback, 0);
        rr.reply_ok({ { "uuid", boost::uuids::to_string(id) } });
    });
}

MAA_TOOLKIT_NS_END
