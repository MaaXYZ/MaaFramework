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

    bool save_on_error() const { return save_on_error_; }

    int draw_quality() const { return draw_quality_; }

    size_t reco_image_cache_limit() const { return reco_image_cache_limit_; }

private:
    OptionMgr() = default;

private:
    bool set_log_dir(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_save_draw(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_stdout_level(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_debug_mode(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_save_on_error(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_draw_quality(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_reco_image_cache_limit(MaaOptionValue value, MaaOptionValueSize val_size);

private:
    std::filesystem::path log_dir_;
    bool save_draw_ = false;
    bool debug_mode_ = false;
    bool save_on_error_ = false;
    int draw_quality_ = 85;
    size_t reco_image_cache_limit_ = 4096;
};

MAA_GLOBAL_NS_END
