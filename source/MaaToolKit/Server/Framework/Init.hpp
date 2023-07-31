#pragma once

#include "../ApiDispatcher.h"
#include "MaaFramework/MaaAPI.h"

MAA_TOOLKIT_NS_BEGIN

void init_maa_framework(ApiDispatcher& disp)
{
    disp.register_route(ApiDispatcher::Method::get, "/api/version", [](auto& rr, auto, auto) {
        rr.reply_ok({ { "version", MaaVersion() } });
    });
    disp.register_route(ApiDispatcher::Method::get, "/test/png", [](auto& rr, auto, auto) {
        std::fstream f("test.png", std::ios_base::binary | std::ios_base::in);
        f.seekg(0, std::ios_base::end);
        size_t l = f.tellg();
        char* p = new char[l];
        f.seekg(0);
        f.read(p, l);
        rr.reply_file(p, l, "image/png");
        delete[] p;
    });
}

MAA_TOOLKIT_NS_END
