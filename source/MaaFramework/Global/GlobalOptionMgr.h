#pragma once

#include <chrono>
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
    const std::filesystem::path& log_dir() const { return log_dir_; }

    bool save_draw() const { return save_draw_; }

    bool show_hit_draw() const { return show_hit_draw_; }

    bool recording() const { return recording_; }

    bool debug_mode() const { return debug_mode_; }

private:
    GlobalOptionMgr() = default;

private:
    bool set_log_dir(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_save_draw(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_show_hit_draw(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_recording(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_stdout_level(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_debug_mode(MaaOptionValue value, MaaOptionValueSize val_size);

private:
    std::filesystem::path log_dir_;
    bool save_draw_ = false;
    bool show_hit_draw_ = false;
    bool recording_ = false;
    bool debug_mode_ = false;
};

MAA_NS_END
