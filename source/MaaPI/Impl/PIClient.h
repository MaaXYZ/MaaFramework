#pragma once

#include "Impl/PIRuntime.h"
#include "PI/Types.h"

MAA_PI_NS_BEGIN

class PIClient : public MaaPIClient
{
public:
    virtual void setup(std::string_view locale, MaaPIClientHandler handler, void* handler_arg) override;
    virtual bool perform(MaaPIRuntime* rt, MaaPIClientAction action) override;

private:
    std::string query_i18n(const I18nString& str, std::string def);
    bool inner_perform(PIRuntime* rt, MaaPIClientAction action);

    std::string locale_ = "zh-Hans";
    MaaPIClientHandler handler_ = nullptr;
    void* handler_arg_ = nullptr;
};

MAA_PI_NS_END
