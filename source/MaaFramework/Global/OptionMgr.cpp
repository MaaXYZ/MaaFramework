#include "OptionMgr.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_GLOBAL_NS_BEGIN

bool OptionMgr::set_option(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    switch (key) {
    case MaaGlobalOption_LogDir:
        return set_log_dir(value, val_size);
    case MaaGlobalOption_SaveDraw:
        return set_save_draw(value, val_size);
    case MaaGlobalOption_StdoutLevel:
        return set_stdout_level(value, val_size);
    case MaaGlobalOption_DebugMode:
        return set_debug_mode(value, val_size);
    case MaaGlobalOption_SaveOnError:
        return set_save_on_error(value, val_size);
    case MaaGlobalOption_DrawQuality:
        return set_draw_quality(value, val_size);
    case MaaGlobalOption_RecoImageCacheLimit:
        return set_reco_image_cache_limit(value, val_size);
    case MaaGlobalOption_RecoImageRaw:
        return set_reco_image_raw(value, val_size);
    case MaaGlobalOption_RecoImageDraws:
        return set_reco_image_draws(value, val_size);
    case MaaGlobalOption_RecoImageCacheBytes:
        return set_reco_image_cache_bytes(value, val_size);
    case MaaGlobalOption_CaptureConfig:
        return set_capture_config(value, val_size);
    default:
        LogError << "Unknown key" << VAR(key) << VAR(value);
        return false;
    }
}

bool OptionMgr::set_log_dir(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    std::string_view str_path(reinterpret_cast<const char*>(value), val_size);
    log_dir_ = MAA_NS::path(str_path);

    LogInfo << "Set log dir" << VAR(log_dir_);

    MAA_LOG_NS::Logger::get_instance().start_logging(log_dir_);

    return true;
}

bool OptionMgr::set_save_draw(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    if (val_size != sizeof(bool)) {
        LogError << "Invalid value size" << VAR(val_size);
        return false;
    }

    save_draw_ = *reinterpret_cast<const bool*>(value);

    LogInfo << "Set save draw" << VAR(save_draw_);

    return true;
}

bool OptionMgr::set_stdout_level(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    if (val_size != sizeof(MaaLoggingLevel)) {
        LogError << "Invalid value size" << VAR(val_size);
        return false;
    }

    MaaLoggingLevel level = *reinterpret_cast<const MaaLoggingLevel*>(value);

    LogInfo << "Set log stdout level" << VAR(level);

    MAA_LOG_NS::Logger::get_instance().set_stdout_level(static_cast<MAA_LOG_NS::level>(level));

    return true;
}

bool OptionMgr::set_debug_mode(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    if (val_size != sizeof(bool)) {
        LogError << "Invalid value size" << VAR(val_size);
        return false;
    }

    debug_mode_ = *reinterpret_cast<const bool*>(value);

    LogInfo << "Set debug message" << VAR(debug_mode_);

    return true;
}

bool OptionMgr::set_save_on_error(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    if (val_size != sizeof(bool)) {
        LogError << "Invalid value size" << VAR(val_size);
        return false;
    }

    save_on_error_ = *reinterpret_cast<const bool*>(value);

    LogInfo << "Set save on error" << VAR(save_on_error_);

    return true;
}

bool OptionMgr::set_draw_quality(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    if (val_size != sizeof(int32_t)) {
        LogError << "Invalid value size" << VAR(val_size);
        return false;
    }

    int32_t quality = *reinterpret_cast<const int32_t*>(value);
    if (quality < 0 || quality > 100) {
        LogError << "Invalid quality value, should be in [0, 100]" << VAR(quality);
        return false;
    }

    draw_quality_ = quality;

    LogInfo << "Set draw quality" << VAR(draw_quality_);

    return true;
}

bool OptionMgr::set_reco_image_cache_limit(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    if (val_size != sizeof(size_t)) {
        LogError << "Invalid value size" << VAR(val_size);
        return false;
    }

    reco_image_cache_limit_ = *reinterpret_cast<const size_t*>(value);

    LogInfo << "Set reco image cache limit" << VAR(reco_image_cache_limit_.load());

    return true;
}

bool OptionMgr::reco_image_raw() const
{
    const int value = reco_image_raw_.load();
    return value < 0 ? debug_mode() : value != 0;
}

bool OptionMgr::reco_image_draws() const
{
    const int value = reco_image_draws_.load();
    if (value >= 0) {
        return value != 0;
    }
#ifdef MAA_DEBUG
    return true;
#else
    return save_draw() || debug_mode();
#endif
}

bool OptionMgr::set_reco_image_raw(MaaOptionValue value, MaaOptionValueSize val_size)
{
    if (!value || val_size != sizeof(bool)) {
        return false;
    }
    reco_image_raw_ = *static_cast<const bool*>(value);
    LogInfo << "Set reco image raw" << VAR(reco_image_raw_.load());
    return true;
}

bool OptionMgr::set_reco_image_draws(MaaOptionValue value, MaaOptionValueSize val_size)
{
    if (!value || val_size != sizeof(bool)) {
        return false;
    }
    reco_image_draws_ = *static_cast<const bool*>(value);
    LogInfo << "Set reco image draws" << VAR(reco_image_draws_.load());
    return true;
}

bool OptionMgr::set_reco_image_cache_bytes(MaaOptionValue value, MaaOptionValueSize val_size)
{
    if (!value || val_size != sizeof(size_t)) {
        return false;
    }
    reco_image_cache_bytes_ = *static_cast<const size_t*>(value);
    LogInfo << "Set reco image cache bytes" << VAR(reco_image_cache_bytes_.load());
    return true;
}

bool OptionMgr::set_capture_config(MaaOptionValue value, MaaOptionValueSize val_size)
{
    if (!value || val_size != sizeof(bool)) {
        return false;
    }
    capture_config_ = *static_cast<const bool*>(value);
    LogInfo << "Set capture config" << VAR(capture_config_.load());
    return true;
}

MAA_GLOBAL_NS_END
