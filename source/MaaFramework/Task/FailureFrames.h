#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "Common/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "MaaUtils/NoWarningCVMat.hpp"

MAA_NS_BEGIN

// Experimental lossless history. The byte pool also covers frames held by the writer.
class FailureFrames
{
public:
    ~FailureFrames();
    void begin();
    void end();
    void observe(const cv::Mat& image, MaaRecoId reco_id, const std::string& node, bool failure = false);
    void mark_action();
    void save(const std::filesystem::path& directory, const std::string& node);
    size_t frame_count() const;
    size_t retained_bytes() const;

private:
    struct Pixels
    {
        cv::Mat image;
        std::shared_ptr<std::atomic_size_t> budget;
        size_t bytes = 0;
        ~Pixels();
    };

    struct Frame
    {
        std::shared_ptr<Pixels> pixels;
        int64_t first_ms = 0;
        int64_t last_ms = 0;
        MaaRecoId reco_id = MaaInvalidId;
        MaaRecoId last_reco_id = MaaInvalidId;
        std::string node;
        std::string last_node;
        bool failure = false;
        bool recognition = true;
    };

    struct Export
    {
        std::filesystem::path directory;
        std::string node;
        std::vector<Frame> frames;
        size_t dropped = 0;
    };

    void write(const Export& job);
    void work();
    bool wait_idle(std::chrono::milliseconds timeout);
    static constexpr size_t kMaxBytes = 32 * 1024 * 1024;
    static constexpr size_t kMaxFrames = 64;

    mutable std::mutex mutex_;
    std::shared_ptr<std::atomic_size_t> bytes_ = std::make_shared<std::atomic_size_t>(0);
    bool active_ = false;
    bool after_action_pending_ = false;
    bool stopping_ = false;
    bool writing_ = false;
    size_t dropped_ = 0;
    std::deque<Frame> frames_;
    std::optional<Frame> before_action_;
    std::optional<Frame> after_action_;
    std::optional<Export> pending_;
    std::condition_variable wake_;
    // Drains pending exports before exiting; joined in the destructor.
    std::thread writer_;
};

MAA_NS_END
