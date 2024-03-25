#pragma once

#include "./type.hpp"

#include "utils/phony.hpp"

#include "../info.hpp"

LHGArgHandle(maa::func_type_CustomActionRun::_0_sync_context, scope);
LHGArgHandle(maa::func_type_CustomRecognizerAnalyze::_0_sync_context, scope);
LHGArgHandle(maa::func_type_CustomRecognizerAnalyze::_1_image, scope);

LHGArgToJsonBegin(maa::func_type_CustomActionRun, _3_cur_box, false)
    auto rec = std::get<index>(arg);
    value = pri_maa::from_rect(rec);
    LHGArgToJsonMiddle()
    pri_maa::schema_rect(b);
LHGArgToJsonEnd()

LHGArgOutput(maa::func_type_CustomRecognizerAnalyze::_5_out_box);
LHGArgOutput(maa::func_type_CustomRecognizerAnalyze::_6_out_detail);

LHGArgFromJsonBegin(maa::func_type_CustomRecognizerAnalyze, _5_out_box, false)
    auto rec = std::get<index>(arg);
    pri_maa::to_rect(value, rec);
    LHGArgFromJsonMiddle()
    pri_maa::schema_rect(b);
LHGArgFromJsonEnd()

LHGArgFromJsonBegin(maa::func_type_CustomRecognizerAnalyze, _6_out_detail, false)
    auto det = std::get<index>(arg);
    std::string detail = value.as_string();
    MaaSetStringEx(det, detail.c_str(), detail.size());
    LHGArgFromJsonMiddle()
    b.type("string");
LHGArgFromJsonEnd()

LHGArgGetContextBegin(maa::callback_CustomActionRun)
    auto ptr =
        reinterpret_cast<pri_maa::custom_action_context*>(std::get<callback_tag::context>(arg));
    return ptr->run.get();
LHGArgGetContextEnd()

LHGArgGetContextBegin(maa::callback_CustomActionStop)
    auto ptr =
        reinterpret_cast<pri_maa::custom_action_context*>(std::get<callback_tag::context>(arg));
    return ptr->stop.get();
LHGArgGetContextEnd()
