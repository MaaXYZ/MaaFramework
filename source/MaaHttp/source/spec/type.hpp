#pragma once

#include <memory>

#include "MaaFramework/MaaDef.h"
#include "utils/phony.hpp"

namespace lhg
{

namespace pri_maa
{

struct custom_action_context
{
    std::shared_ptr<lhg::callback::context_info> run;
    std::shared_ptr<lhg::callback::context_info> stop;
};

inline json::object from_rect(MaaRectHandle rec)
{
    return {
        { "x", rec->x },
        { "y", rec->y },
        { "width", rec->width },
        { "height", rec->height },
    };
}

inline bool to_rect(const json::value& value, MaaRectHandle rec)
{
    rec->x = value.at("x").as_integer();
    rec->y = value.at("y").as_integer();
    rec->width = value.at("width").as_integer();
    rec->height = value.at("height").as_integer();
    return true;
}

inline void schema_rect(schema::Builder& b)
{
    b.type("object").prop({ { "x", schema::Builder().type("number").obj },
                            { "y", schema::Builder().type("number").obj },
                            { "width", schema::Builder().type("number").obj },
                            { "height", schema::Builder().type("number").obj } });
}

} // namespace pri_maa

} // namespace lhg
