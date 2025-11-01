#pragma once

#include <chrono>
#include <filesystem>
#include <string_view>

#include "Common/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "MaaUtils/SingletonHolder.hpp"

MAA_GLOBAL_NS_BEGIN

class OptionMgr : public SingletonHolder<OptionMgr>
{
public:
    friend class SingletonHolder<OptionMgr>;

public:
    virtual ~OptionMgr() = default;
    bool set_option(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size);

public:
    const std::filesystem::path& log_dir() const { return log_dir_; }

    bool save_draw() const { return save_draw_; }

    bool debug_mode() const { return debug_mode_; }

private:
    OptionMgr() = default;

private:
    bool set_log_dir(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_save_draw(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_stdout_level(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_debug_mode(MaaOptionValue value, MaaOptionValueSize val_size);

private:
    std::filesystem::path log_dir_;
    bool save_draw_ = false;
    bool debug_mode_ = false;
};

MAA_GLOBAL_NS_END
