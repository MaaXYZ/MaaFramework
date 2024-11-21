#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include <onnxruntime/onnxruntime_cxx_api.h>

#include "Conf/Conf.h"
#include "Utils/NonCopyable.hpp"

MAA_RES_NS_BEGIN

class ONNXResMgr : public NonCopyable
{
public:
    inline static const std::filesystem::path kClassifierDir = "classify";
    inline static const std::filesystem::path kDetectorDir = "detect";

    ONNXResMgr();

public:
    void set_cpu();
    void set_cuda(int device_id);
    void set_dml(int device_id);
    void set_coreml(uint32_t coreml_flag);

    bool lazy_load(const std::filesystem::path& path, bool is_base);
    void clear();

public:
    std::shared_ptr<Ort::Session> classifier(const std::string& name);
    std::shared_ptr<Ort::Session> detector(const std::string& name);
    const Ort::MemoryInfo& memory_info() const;

private:
    std::shared_ptr<Ort::Session> load(const std::string& name, const std::vector<std::filesystem::path>& roots);

    std::vector<std::filesystem::path> classifier_roots_;
    std::vector<std::filesystem::path> detector_roots_;

    Ort::Env env_;
    Ort::SessionOptions options_;
    Ort::MemoryInfo memory_info_;

    std::unordered_map<std::string, std::shared_ptr<Ort::Session>> classifiers_;
    std::unordered_map<std::string, std::shared_ptr<Ort::Session>> detectors_;
};

MAA_RES_NS_END
