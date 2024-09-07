#include "Impl/PIClient.h"

#include <MaaFramework/MaaAPI.h>
#include <format>
#include <functional>
#include <meojson/json.hpp>

#include "Buffer/StringBuffer.hpp"
#include "MaaFramework/MaaAPI.h"
#include "MaaPI/MaaPIDef.h"
#include "Utils/JsonExt.hpp"
#include "Utils/ScopeLeave.hpp"

MAA_PI_NS_BEGIN

void PIClient::setup(std::string_view locale, MaaPIClientHandler handler, void* handler_arg)
{
    locale_ = locale;
    handler_ = handler;
    handler_arg_ = handler_arg;
}

bool PIClient::perform(MaaPIRuntime* rt, MaaPIClientAction action)
{
    auto runtime = dynamic_cast<PIRuntime*>(rt);
    if (!runtime) {
        return false;
    }

    if (!(MaaPIClientAction_DirectBegin <= action && action <= MaaPIClientAction_DirectEnd)) {
        return false;
    }

    return inner_perform(runtime, action);
}

std::string PIClient::query_i18n(const I18nString& str, std::string def)
{
    auto it = str.find(locale_);
    if (it != str.end()) {
        return it->second;
    }
    else {
        return def;
    }
}

bool PIClient::inner_perform(PIRuntime* rt, MaaPIClientAction action)
{
    MaaSize count = 0;

    MaaStringListBuffer* choice = MaaStringListBufferCreate();
    MaaStringListBuffer* details = MaaStringListBufferCreate();

    OnScopeLeave([&]() {
        MaaStringListBufferDestroy(choice);
        MaaStringListBufferDestroy(details);
    });

    std::function<bool(MaaSize choice)> result;
    switch (action) {
    case MaaPIClientAction_SelectResource: {
        count = rt->data_->resource_order_.size();
        for (const auto& res_name : rt->data_->resource_order_) {
            const auto& res = rt->data_->resource_[res_name];
            choice->append(std::format("{}: {}", res.name, query_i18n(res.name_i18n, res.name)));
            details->append((json::object {
                                 { "name", res.name },
                                 { "name_i18n", json::object { res.name_i18n } },
                                 { "paths", json::array { res.paths } },
                             })
                                .to_string());
        }
        result = [&](MaaSize choice) -> bool {
            rt->config_->resource_ = &rt->data_->resource_[rt->data_->resource_order_[choice]];
            return true;
        };
        break;
    }
    default:
        return false;
    }

    auto cho = handler_(this, action, choice, details, handler_arg_);
    if (cho < 0) {
        return false;
    }

    auto ucho = static_cast<MaaSize>(cho);
    if (ucho >= count) {
        return false;
    }

    return result(ucho);
}

MAA_PI_NS_END
