#pragma once

#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

#include "Conf/Conf.h"

MAA_TOOLKIT_NS_BEGIN

enum class TextTransferMode
{
    StdIO = 1,
    FileIO,
};
inline std::ostream& operator<<(std::ostream& os, TextTransferMode mode)
{
    switch (mode) {
    case TextTransferMode::StdIO:
        os << "StdIO";
        break;
    case TextTransferMode::FileIO:
        os << "FileIO";
        break;
    }
    return os;
}

enum class ImageTransferMode
{
    FileIO = 1,
};
inline std::ostream& operator<<(std::ostream& os, ImageTransferMode mode)
{
    switch (mode) {
    case ImageTransferMode::FileIO:
        os << "FileIO";
        break;
    }
    return os;
}

struct ExecData
{
    std::string name;
    std::filesystem::path exec_path;
    std::vector<std::string> exec_args;
    TextTransferMode text_mode;
    ImageTransferMode image_mode;
};

MAA_TOOLKIT_NS_END
