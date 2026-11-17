#pragma once

#include <filesystem>

bool ocr_regression(const std::filesystem::path& testset_dir, const std::filesystem::path& report_dir);
