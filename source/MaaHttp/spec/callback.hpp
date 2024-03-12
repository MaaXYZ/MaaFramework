#pragma once

#include "./type.hpp"
#include "./utils.hpp"

#include "MaaFramework/Utility/MaaBuffer.h"
#include "utils/phony.hpp"

#include "../info.hpp"

namespace lhg::callback
{

__CALLBACK_DECLARE_HANDLE_OPER(maa::func_type_CustomActionRun::_0_sync_context, scope)
__CALLBACK_DECLARE_ARG_TO_JSON_BEGIN(maa::func_type_CustomActionRun, _3_cur_box)
auto rec = std::get<maa::func_type_CustomActionRun::_3_cur_box::index>(arg);
req[name] = {
    { "x", rec->x },
    { "y", rec->y },
    { "width", rec->width },
    { "height", rec->height },
};
__CALLBACK_DECLARE_ARG_TO_JSON_END()

__CALLBACK_DECLARE_HANDLE_OPER(maa::func_type_CustomRecognizerAnalyze::_0_sync_context, scope)
__CALLBACK_DECLARE_HANDLE_OPER(maa::func_type_CustomRecognizerAnalyze::_1_image, scope)
__CALLBACK_DECLARE_INPUT(maa::func_type_CustomRecognizerAnalyze::_5_out_box, false)
__CALLBACK_DECLARE_INPUT(maa::func_type_CustomRecognizerAnalyze::_6_out_detail, false)
__CALLBACK_DECLARE_OUTPUT(maa::func_type_CustomRecognizerAnalyze::_5_out_box, true)
__CALLBACK_DECLARE_OUTPUT(maa::func_type_CustomRecognizerAnalyze::_6_out_detail, true)

__CALLBACK_DECLARE_JSON_TO_ARG_BEGIN(maa::func_type_CustomRecognizerAnalyze, _5_out_box)
auto rec = std::get<maa::func_type_CustomRecognizerAnalyze::_5_out_box::index>(arg);
rec->x = value.at("x").as_integer();
rec->y = value.at("y").as_integer();
rec->width = value.at("width").as_integer();
rec->height = value.at("height").as_integer();
return true;
__CALLBACK_DECLARE_ARG_TO_JSON_END()

__CALLBACK_DECLARE_JSON_TO_ARG_BEGIN(maa::func_type_CustomRecognizerAnalyze, _6_out_detail)
auto det = std::get<maa::func_type_CustomRecognizerAnalyze::_6_out_detail::index>(arg);
std::string detail = value.as_string();
MaaSetStringEx(det, detail.c_str(), detail.size());
return true;
__CALLBACK_DECLARE_ARG_TO_JSON_END()

template <>
struct get_context<maa::callback_CustomActionRun, true>
{
    using callback_tag = maa::callback_CustomActionRun;
    using func_type = typename callback_tag::type;
    using arg_tuple = typename func_type::args;
    using call_arg_tuple = convert_arg_type<arg_tuple>;
    static context_info* get(const call_arg_tuple& arg)
    {
        auto ptr = reinterpret_cast<pri_maa::custom_action_context*>(std::get<callback_tag::context>(arg));
        return ptr->run.get();
    }
};

template <>
struct get_context<maa::callback_CustomActionStop, true>
{
    using callback_tag = maa::callback_CustomActionStop;
    using func_type = typename callback_tag::type;
    using arg_tuple = typename func_type::args;
    using call_arg_tuple = convert_arg_type<arg_tuple>;
    static context_info* get(const call_arg_tuple& arg)
    {
        auto ptr = reinterpret_cast<pri_maa::custom_action_context*>(std::get<callback_tag::context>(arg));
        return ptr->stop.get();
    }
};

}; // namespace lhg::callback
