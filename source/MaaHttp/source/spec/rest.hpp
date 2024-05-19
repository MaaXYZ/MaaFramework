#pragma once

#include "helper.hpp"
#include "type.hpp"

#include "utils/phony.hpp"

#include "../info.hpp"

namespace lhg::lifecycle
{

// MaaCreate

#define CURF maa::func_type_MaaCreate

LHGHandle(CURF::ret, alloc);

LHGCallback(maa::callback_MaaAPICallback, CURF::_0_callback, CURF::_1_callback_arg, true);

#undef CURF

// MaaDestroy

#define CURF maa::func_type_MaaDestroy

LHGHandle(CURF::_0_inst, free);

#undef CURF

// CustomRecognizerAnalyze

#define CURF maa::func_type_CustomRecognizerAnalyze

LHGOuterState(CURF::_0_sync_context, HandleManager<MaaSyncContextHandle>::ScopedHandle);
LHGOuterState(CURF::_1_image, HandleManager<MaaImageBufferHandle>::ScopedHandle);
LHGOuterState(CURF::_5_out_box, MaaRect);

LHGHandle(CURF::_0_sync_context, scope);
LHGHandle(CURF::_1_image, scope);

LHGOutput(CURF::_5_out_box);
LHGOutput(CURF::_6_out_detail);

MaaDeclRectIn(_5_out_box, false);

LHGInherit (json_to_arg, CURF::_6_out_detail, false) {
    bool convert()
    {
        if (!req.contains(name)) {
            return false;
        }
        std::string detail = req.at(name).as_string();
        MaaSetStringEx(std::get<index>(arg), detail.c_str(), detail.size());
        return true;
    }
};

LHGSchema(json_to_arg_schema, CURF::_6_out_detail, false, "string");

#undef CURF

// MaaRegisterCustomRecognizer

#define CURF maa::func_type_MaaRegisterCustomRecognizer

LHGHide(CURF::_3_recognizer_arg);

LHGInherit (json_to_arg, CURF::_2_recognizer, true) {
    bool convert()
    {
        if (!req.contains(name)) {
            return false;
        }
        std::string id = req.at(name).as_string();
        auto manager = provider.get<CallbackManager<maa::callback_CustomRecognizerAnalyze>, void>();
        auto ctx = manager->query(id);
        if (!ctx.get()) {
            return false;
        }
        static MaaCustomRecognizerAPI api = {
            callback::create_callback<maa::callback_CustomRecognizerAnalyze>()
        };
        std::get<index>(arg) = &api;
        std::get<_3_recognizer_arg::index>(arg) = ctx.get();
        return true;
    }
};

LHGSchema(json_to_arg_schema, CURF::_2_recognizer, true, "string");

#undef CURF

// CustomActionRun

#define CURF maa::func_type_CustomActionRun

LHGOuterState(CURF::_0_sync_context, HandleManager<MaaSyncContextHandle>::ScopedHandle);
LHGOuterState(CURF::_3_cur_box, MaaRect);

LHGHandle(CURF::_0_sync_context, scope);

MaaDeclRectOut(_3_cur_box, false);

#undef CURF

// MaaRegisterCustomAction

#define CURF maa::func_type_MaaRegisterCustomAction

LHGInherit (get_context, maa::callback_CustomActionRun) {
    void get()
    {
        ctx = reinterpret_cast<pri_maa::custom_action_context*>(std::get<context>(arg))->run.get();
    }
};

LHGInherit (get_context, maa::callback_CustomActionStop) {
    void get()
    {
        ctx = reinterpret_cast<pri_maa::custom_action_context*>(std::get<context>(arg))->stop.get();
    }
};

LHGHide(CURF::_3_action_arg);

LHGInherit (json_to_arg, CURF::_2_action, true) {
    bool convert()
    {
        if (!req.contains(name)) {
            return false;
        }
        json::object ids = req.at(name).as_object();
        std::string run_id = ids["run"].as_string();
        std::string stop_id = ids["stop"].as_string();
        auto run_manager = provider.get<CallbackManager<maa::callback_CustomActionRun>, void>();
        auto run_ctx = run_manager->query(run_id);
        if (!run_ctx.get()) {
            return false;
        }
        auto stop_manager = provider.get<CallbackManager<maa::callback_CustomActionStop>, void>();
        auto stop_ctx = stop_manager->query(stop_id);
        if (!stop_ctx.get()) {
            return false;
        }
        auto ctx = provider.get<DataManager, void>()->alloc<pri_maa::custom_action_context>(
            std::vector<std::string> { run_id, stop_id });
        ctx->run = run_ctx;
        ctx->stop = stop_ctx;
        static MaaCustomActionAPI api = {
            callback::create_callback<maa::callback_CustomActionRun>(),
            callback::create_callback<maa::callback_CustomActionStop>()
        };
        std::get<index>(arg) = &api;
        std::get<_3_action_arg::index>(arg) = ctx;
        return true;
    }
};

LHGInherit (json_to_arg_schema, CURF::_2_action, true) {
    void schema()
    {
        b.type("object").prop({ { "run", lhg::schema::Builder().type("string").obj },
                                { "stop", lhg::schema::Builder().type("string").obj } });
        res[name] = b.obj;
    }
};

#undef CURF

// MaaQueryRecognitionDetail

#define CURF maa::func_type_MaaQueryRecognitionDetail

LHGOuterState(CURF::_1_hit, MaaBool);
LHGOuterState(CURF::_2_hit_box, MaaRect);
LHGOuterState(CURF::_3_detail_json, MaaStringBufferHandle);

LHGOutput(CURF::_1_hit);
LHGOutput(CURF::_2_hit_box);

LHGInherit (pre_process, CURF::_1_hit) {
    void process()
    {
        std::get<index>(arg) = &std::get<index>(state);
    }
};

LHGInherit (arg_to_json, CURF::_1_hit, true) {
    void convert()
    {
        res[name] = !!std::get<index>(state);
    }
};

LHGSchema(arg_to_json_schema, CURF::_1_hit, true, "boolean");

MaaDeclRectOut(_2_hit_box, true);

MaaDeclStringBuffer(_3_detail_json, true);

#undef CURF

// MaaQueryNodeDetail

#define CURF maa::func_type_MaaQueryNodeDetail

LHGOuterState(CURF::_1_reco_id, MaaRecoId);
LHGOuterState(CURF::_2_run_completed, MaaBool);

LHGOutput(CURF::_1_reco_id);

LHGInherit (pre_process, CURF::_1_reco_id) {
    void process()
    {
        std::get<index>(arg) = &std::get<index>(state);
    }
};

LHGInherit (arg_to_json, CURF::_1_reco_id, true) {
    void convert()
    {
        res[name] = std::get<index>(state);
    }
};

LHGSchema(arg_to_json_schema, CURF::_1_reco_id, true, "number");

LHGOutput(CURF::_2_run_completed);

LHGInherit (pre_process, CURF::_2_run_completed) {
    void process()
    {
        std::get<index>(arg) = &std::get<index>(state);
    }
};

LHGInherit (arg_to_json, CURF::_2_run_completed, true) {
    void convert()
    {
        res[name] = !!std::get<index>(state);
    }
};

LHGSchema(arg_to_json_schema, CURF::_2_run_completed, true, "boolean");

#undef CURF

// MaaQueryTaskDetail

#define CURF maa::func_type_MaaQueryTaskDetail

LHGOuterState(CURF::_1_node_id_list, std::vector<MaaNodeId>);
LHGOuterState(CURF::_2_node_id_list_size, MaaSize);

LHGOutput(CURF::_1_node_id_list);
LHGHide(CURF::_2_node_id_list_size);

LHGInherit (pre_process, CURF::_1_node_id_list) {
    void process()
    {
        if (!req.contains(_0_task_id::name)) {
            return;
        }
        // 不能直接使用arg, 此时还没json_to_arg
        auto v = req.at(_0_task_id::name).as<MaaTaskId>();
        MaaSize& sz = std::get<_2_node_id_list_size::index>(state);
        if (!MaaQueryTaskDetail(v, nullptr, &sz)) {
            return;
        }
        auto& lst = std::get<index>(state);
        lst.resize(sz);
        std::get<index>(arg) = lst.data();
        std::get<_2_node_id_list_size::index>(arg) = &std::get<_2_node_id_list_size::index>(state);
    }
};

LHGInherit (arg_to_json, CURF::_1_node_id_list, true) {
    void convert()
    {
        res[name] = std::get<index>(state);
    }
};

LHGInherit (arg_to_json_schema, CURF::_1_node_id_list, true) {
    void schema()
    {
        b.type("array").items(lhg::schema::Builder().type("number").obj);
        res[name] = b.obj;
    }
};

#undef CURF

}
