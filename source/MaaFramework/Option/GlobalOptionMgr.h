#pragma once

#include <filesystem>
#include <string_view>

#include "Conf/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "Utils/SingletonHolder.hpp"

MAA_NS_BEGIN

class GlobalOptionMgr : public SingletonHolder<GlobalOptionMgr>
{
public:
    friend class SingletonHolder<GlobalOptionMgr>;

public:
    virtual ~GlobalOptionMgr() = default;
    bool set_option(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size);

public:
    const std::filesystem::path& logging_path() const { return logging_path_; }
    bool debug_mode() const { return debug_mode_; }
    bool recording() const { return recording_; }

private:
    GlobalOptionMgr() = default;

private:
    bool set_logging(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_debug_mode(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_recording(MaaOptionValue value, MaaOptionValueSize val_size);

private:
    std::filesystem::path logging_path_;
    bool debug_mode_ = false;
    bool recording_ = false;
};

MAA_NS_END
