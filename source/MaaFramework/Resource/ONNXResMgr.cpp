#include "ONNXResMgr.h"

#include <filesystem>
#include <ranges>

#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_RES_NS_BEGIN

bool ONNXResMgr::lazy_load(const std::filesystem::path& path, bool is_base)
{
    LogFunc << VAR(path) << VAR(is_base);

    if (is_base) {
        clear();
    }

    classifier_roots_.emplace_back(path / kClassifierDir);
    detector_roots_.emplace_back(path / kDetectorDir);

    return true;
}

void ONNXResMgr::clear()
{
    LogFunc;

    classifier_roots_.clear();
    detector_roots_.clear();
    classifiers_.clear();
    detectors_.clear();
}

std::shared_ptr<Ort::Session> ONNXResMgr::classifier(const std::string& name) const
{
    if (auto iter = classifiers_.find(name); iter != classifiers_.end()) {
        return iter->second;
    }

    auto session = load(name, classifier_roots_);
    if (session) {
        classifiers_.emplace(name, session);
    }

    return session;
}

std::shared_ptr<Ort::Session> ONNXResMgr::detector(const std::string& name) const
{
    if (auto iter = detectors_.find(name); iter != detectors_.end()) {
        return iter->second;
    }

    auto session = load(name, detector_roots_);
    if (session) {
        detectors_.emplace(name, session);
    }

    return session;
}

std::shared_ptr<Ort::Session>
    ONNXResMgr::load(const std::string& name, const std::vector<std::filesystem::path>& roots) const
{
    LogFunc << VAR(name) << VAR(roots);

    for (const auto& root : roots | std::views::reverse) {
        auto path = root / MAA_NS::path(name);
        if (!std::filesystem::exists(path)) {
            continue;
        }

        LogTrace << VAR(path);
        Ort::Session session(m_env, path.c_str(), m_options);
        return std::make_shared<Ort::Session>(std::move(session));
    }

    return nullptr;
}

MAA_RES_NS_END