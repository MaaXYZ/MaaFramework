#include "ActionExecAgent.h"

#include <functional>

#include "MaaFramework/MaaAPI.h"
#include "Utils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

ActionExecAgent::ActionExecAgent()
{
    custom_action_.run = &ActionExecAgent::maa_api_run;
    custom_action_.stop = &ActionExecAgent::maa_api_stop;
}

bool ActionExecAgent::register_for_maa_inst(
    MaaTasker* handle,
    std::string_view name,
    ExecData& executor)
{
    return MaaRegisterCustomAction(
        handle,
        name.data(),
        &custom_action_,
        reinterpret_cast<void*>(&executor));
}

bool ActionExecAgent::unregister_for_maa_inst(MaaTasker* handle, std::string_view name)
{
    return MaaUnregisterCustomAction(handle, name.data());
}

bool ActionExecAgent::run(
    ExecData& data,
    MaaContext* context,
    std::string_view task_name,
    std::string_view custom_action_param,
    MaaRect* cur_box,
    std::string_view cur_rec_detail)
{
    LogFunc << VAR(data.name) << VAR_VOIDP(context) << VAR(task_name)
            << VAR(custom_action_param) << VAR_VOIDP(cur_box) << VAR(cur_rec_detail);

    std::string handle_arg = arg_cvt_.context_to_arg(context);
    std::string box_arg =
        json::array({ cur_box->x, cur_box->y, cur_box->width, cur_box->height }).to_string();

    std::vector<std::string> extra_args = { handle_arg,
                                            std::string(task_name),
                                            std::string(custom_action_param),
                                            box_arg,
                                            std::string(cur_rec_detail) };
    std::vector<std::string> args = data.exec_args;
    args.insert(
        args.end(),
        std::make_move_iterator(extra_args.begin()),
        std::make_move_iterator(extra_args.end()));

    auto output_opt = run_executor(data.exec_path, args);
    if (!output_opt) {
        LogError << "run_executor failed" << VAR(data.exec_path) << VAR(data.exec_args);
        return false;
    }

    return true;
}

void ActionExecAgent::stop(ExecData& data)
{
    LogFunc << VAR(data.name);

    // TODO
}

MaaBool ActionExecAgent::maa_api_run(
    MaaContext* context,
    const char* task_name,
    const char* custom_action_param,
    MaaRect* cur_box,
    const char* cur_rec_detail,
    MaaTransparentArg trans_arg)
{
    auto* data = static_cast<ExecData*>(trans_arg);
    if (!data) {
        LogError << "data is nullptr" << VAR(trans_arg);
        return false;
    }

    return get_instance()
        .run(*data, context, task_name, custom_action_param, cur_box, cur_rec_detail);
}

void ActionExecAgent::maa_api_stop(MaaTransparentArg trans_arg)
{
    auto* data = static_cast<ExecData*>(trans_arg);
    if (!data) {
        LogError << "data is nullptr" << VAR(trans_arg);
        return;
    }

    get_instance().stop(*data);
}

MAA_TOOLKIT_NS_END