#include "FailureFrames.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <set>

#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"

MAA_NS_BEGIN

namespace
{
uint32_t png_crc32(const std::vector<uchar>& data)
{
    static constexpr auto table = [] {
        std::array<uint32_t, 256> result { };
        for (uint32_t i = 0; i < result.size(); ++i) {
            uint32_t value = i;
            for (int bit = 0; bit < 8; ++bit) {
                value = (value >> 1) ^ ((value & 1) ? 0xedb88320U : 0U);
            }
            result[i] = value;
        }
        return result;
    }();
    uint32_t value = 0xffffffffU;
    for (const auto byte : data) {
        value = table[(value ^ byte) & 0xff] ^ (value >> 8);
    }
    return value ^ 0xffffffffU;
}
} // namespace

FailureFrames::Pixels::~Pixels()
{
    if (budget && bytes) {
        image.release();
        budget->fetch_sub(bytes);
    }
}

FailureFrames::~FailureFrames()
{
    writer_.request_stop();
    wake_.notify_all();
}

void FailureFrames::begin()
{
    std::scoped_lock lock(mutex_);
    frames_.clear();
    before_action_.reset();
    after_action_.reset();
    after_action_pending_ = false;
    dropped_ = 0;
    active_ = true;
}

void FailureFrames::end()
{
    std::scoped_lock lock(mutex_);
    active_ = false;
    frames_.clear();
    before_action_.reset();
    after_action_.reset();
}

void FailureFrames::observe(const cv::Mat& image, MaaRecoId reco_id, const std::string& node, bool failure)
{
    if (image.empty()) {
        return;
    }
    try {
        std::scoped_lock lock(mutex_);
        if (!active_) {
            return;
        }
        const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        const bool equal = !frames_.empty() && image.size() == frames_.back().pixels->image.size()
                           && image.type() == frames_.back().pixels->image.type()
                           && cv::norm(image, frames_.back().pixels->image, cv::NORM_INF) == 0;
        if (!equal) {
            const size_t length = image.total() * image.elemSize();
            if (length > kMaxBytes) {
                ++dropped_;
                return;
            }
            while (frames_.size() > 1 && (bytes_->load() + length > kMaxBytes || frames_.size() >= kMaxFrames)) {
                frames_.pop_front();
            }
            // Pins must not freeze capture when the remaining pool cannot fit
            // a new frame. Keep the pre-action cause for as long as possible.
            for (auto* pin : { &after_action_, &before_action_ }) {
                if (bytes_->load() + length <= kMaxBytes) {
                    break;
                }
                if (*pin) {
                    pin->reset();
                    ++dropped_;
                }
                if (frames_.size() == 1 && frames_.back().pixels.use_count() == 1) {
                    frames_.pop_back();
                }
            }
            if (bytes_->load() + length > kMaxBytes) {
                ++dropped_;
                return;
            }
            auto pixels = std::make_shared<Pixels>();
            pixels->budget = bytes_;
            bytes_->fetch_add(length);
            pixels->bytes = length;
            pixels->image = image.clone();
            frames_.push_back({ pixels, now, now, reco_id, reco_id, node, node, failure, !failure });
        }
        auto& current = frames_.back();
        current.last_ms = now;
        if (reco_id != MaaInvalidId) {
            current.last_reco_id = reco_id;
        }
        current.last_node = node;
        current.failure |= failure;
        current.recognition |= !failure;
        if (after_action_pending_ && !failure) {
            after_action_ = current;
            after_action_pending_ = false;
        }
    }
    catch (const std::exception& error) {
        LogWarn << "failure frame capture failed" << error.what();
    }
}

void FailureFrames::mark_action()
{
    std::scoped_lock lock(mutex_);
    if (!active_ || frames_.empty()) {
        return;
    }
    before_action_ = frames_.back();
    after_action_.reset();
    after_action_pending_ = true;
}

void FailureFrames::save(const std::filesystem::path& directory, const std::string& node)
{
    try {
        std::scoped_lock lock(mutex_);
        if (!active_ || pending_ || (frames_.empty() && !before_action_ && !after_action_)) {
            LogWarn << "failure frame export unavailable or busy" << directory;
            return;
        }
        Export job { directory, node, { }, dropped_ };
        std::set<const Pixels*> seen;
        for (const auto& frame : frames_) {
            seen.insert(frame.pixels.get());
            job.frames.push_back(frame);
        }
        for (const auto* pinned : { &before_action_, &after_action_ }) {
            if (*pinned && seen.insert((*pinned)->pixels.get()).second) {
                job.frames.push_back(**pinned);
            }
        }
        std::ranges::sort(job.frames, { }, &Frame::first_ms);
        pending_ = std::move(job);
        if (!writer_.joinable()) {
            writer_ = std::jthread([this](std::stop_token stop) { work(stop); });
        }
        wake_.notify_one();
    }
    catch (const std::exception& error) {
        LogWarn << "failure frame export scheduling failed" << error.what();
    }
}

void FailureFrames::work(std::stop_token stop)
{
    for (;;) {
        std::optional<Export> job;
        {
            std::unique_lock lock(mutex_);
            wake_.wait(lock, stop, [this] { return pending_.has_value(); });
            if (!pending_) {
                return;
            }
            job = std::move(pending_);
            pending_.reset();
        }
        write(*job);
    }
}

void FailureFrames::write(const Export& job)
{
    auto temporary = job.directory;
    temporary += ".partial";
    try {
        std::filesystem::create_directories(temporary / "frames");
        json::array manifest;
        cv::Mat previous;
        for (size_t i = 0; i < job.frames.size(); ++i) {
            const auto& frame = job.frames[i];
            const auto& image = frame.pixels->image;
            std::vector<uchar> png;
            if (!cv::imencode(".png", image, png, { cv::IMWRITE_PNG_COMPRESSION, 3 })) {
                throw std::runtime_error("PNG encoding failed");
            }
            const std::string filename = std::format("frames/{:04}.png", i);
            std::ofstream output(temporary / filename, std::ios::binary);
            output.exceptions(std::ios::badbit | std::ios::failbit);
            output.write(reinterpret_cast<const char*>(png.data()), png.size());
            output.close();
            cv::Rect box(0, 0, image.cols, image.rows);
            if (!previous.empty() && previous.size() == image.size() && previous.type() == image.type()) {
                cv::Mat diff, mask;
                cv::absdiff(previous, image, diff);
                std::vector<cv::Mat> channels;
                cv::split(diff, channels);
                mask = channels.front();
                for (size_t channel = 1; channel < channels.size(); ++channel) {
                    cv::bitwise_or(mask, channels[channel], mask);
                }
                box = cv::boundingRect(mask);
            }
            manifest.emplace_back(
                json::object { { "file", filename },
                               { "first_time_ms", frame.first_ms },
                               { "last_time_ms", frame.last_ms },
                               { "reco_id", frame.reco_id },
                               { "last_reco_id", frame.last_reco_id },
                               { "node", frame.node },
                               { "last_node", frame.last_node },
                               { "failure", frame.failure },
                               { "source", frame.recognition ? "recognition" : "failure_cached" },
                               { "width", image.cols },
                               { "height", image.rows },
                               { "png_bytes", png.size() },
                               { "crc32", std::format("{:08x}", png_crc32(png)) },
                               { "change_bbox", box } });
            previous = image;
        }
        const json::value document = json::object { { "version", 1 },
                                                    { "failure_node", job.node },
                                                    { "detector", "exact_pixels" },
                                                    { "memory_limit_bytes", kMaxBytes },
                                                    { "dropped_for_memory", job.dropped },
                                                    { "frames", std::move(manifest) } };
        std::ofstream output(temporary / "frames.json");
        output.exceptions(std::ios::failbit | std::ios::badbit);
        output << document;
        output.close();
        std::filesystem::rename(temporary, job.directory);
        LogInfo << "saved failure frames" << job.directory << VAR(job.frames.size());
    }
    catch (const std::exception& error) {
        LogWarn << "failure frame export failed" << job.directory << error.what();
    }
}

size_t FailureFrames::frame_count() const
{
    std::scoped_lock lock(mutex_);
    return frames_.size();
}

size_t FailureFrames::retained_bytes() const
{
    return bytes_->load();
}

MAA_NS_END
