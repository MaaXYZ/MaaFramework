#pragma once

#include "Conf/Conf.h"

#include <string_view>
#include <unordered_map>

#include "ExecAgent/ExecArgConverter.h"
#include "ExecAgentType.h"
#include "MaaFramework/MaaDef.h"

MAA_TOOLKIT_NS_BEGIN

class ExecAgentBase
{
public:
    virtual ~ExecAgentBase() = default;

    bool register_executor(MaaInstanceHandle handle, std::string_view name, std::filesystem::path exec_path,
                           std::vector<std::string> exec_args, TextTransferMode text_mode,
                           ImageTransferMode image_mode);
    bool unregister_executor(MaaInstanceHandle handle, std::string_view name);

protected:
    virtual bool register_for_maa_inst(MaaInstanceHandle handle, std::string_view name) = 0;
    virtual bool unregister_for_maa_inst(MaaInstanceHandle handle, std::string_view name) = 0;

    std::optional<std::string> run_executor(TextTransferMode mode, const std::filesystem::path& exec_path,
                                            const std::vector<std::string>& exec_args);
    std::optional<std::string> run_executor_with_stdio(const std::filesystem::path& exec_path,
                                                       const std::vector<std::string>& exec_args);

protected:
    std::unordered_map</*name*/ std::string, Executor> executors_;

    ExecArgConverter arg_cvt_;
};

MAA_TOOLKIT_NS_END
