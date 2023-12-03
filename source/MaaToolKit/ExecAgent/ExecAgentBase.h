#pragma once

#include "Conf/Conf.h"

#include <string_view>
#include <unordered_map>

#include "ExecAgentType.h"
#include "MaaFramework/MaaDef.h"

MAA_TOOLKIT_NS_BEGIN

class ExecAgentBase
{
public:
    virtual ~ExecAgentBase() = default;

    bool register_executor(MaaInstanceHandle handle, std::string_view name, std::filesystem::path exec_path,
                           std::vector<std::string> exec_params, TextTransferMode text_mode,
                           ImageTransferMode image_mode);
    bool unregister_executor(MaaInstanceHandle handle, std::string_view name);

protected:
    virtual bool register_for_maa_inst(MaaInstanceHandle handle, std::string_view name) = 0;
    virtual bool unregister_for_maa_inst(MaaInstanceHandle handle, std::string_view name) = 0;

protected:
    std::string handle_uuid(MaaInstanceHandle handle);
    MaaInstanceHandle uuid_handle(const std::string& uuid) const;

protected:
    std::unordered_map</*name*/ std::string, Executor> executors_;
    std::unordered_map</*uuid*/ std::string, MaaInstanceHandle> handles_;
};

MAA_TOOLKIT_NS_END
