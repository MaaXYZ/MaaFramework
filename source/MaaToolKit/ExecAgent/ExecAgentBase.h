#pragma once

#include "Conf/Conf.h"

#include <map>
#include <memory>
#include <string_view>

#include "ExecAgent/ExecArgConverter.h"
#include "ExecAgentType.h"
#include "MaaFramework/MaaDef.h"
#include "Utils/IOStream/IOStream.h"

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
    virtual bool register_for_maa_inst(MaaInstanceHandle handle, std::string_view name, ExecData& executor) = 0;
    virtual bool unregister_for_maa_inst(MaaInstanceHandle handle, std::string_view name) = 0;

    std::optional<std::string> run_executor(const std::filesystem::path& exec_path,
                                            const std::vector<std::string>& exec_args, TextTransferMode text_mode,
                                            ImageTransferMode image_mode);

protected:
    std::map<std::string, ExecData> exec_data_;

    ExecArgConverter arg_cvt_;

private:
    std::optional<std::string> handle_ipc(IOStream& ios, ImageTransferMode image_mode);
};

MAA_TOOLKIT_NS_END
