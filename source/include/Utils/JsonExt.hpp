#pragma once

#include <filesystem>
#include <string>

#include <meojson/json.hpp>

#include "Utils/NoWarningCVMat.hpp"

#include "Utils/Codec.h"
#include "Utils/Platform.h"

namespace json::ext
{
template <>
class jsonization<std::filesystem::path>
{
public:
    json::value to_json(const std::filesystem::path& path) const
    {
        return MAA_NS::path_to_utf8_string(path);
    }

    bool check_json(const json::value& json) const { return json.is_string(); }

    bool from_json(const json::value& json, std::filesystem::path& path) const
    {
        path = MAA_NS::path(json.as_string());
        return true;
    }
};

template <>
class jsonization<std::wstring>
{
public:
    json::value to_json(const std::wstring& wstr) const { return MAA_NS::from_u16(wstr); }

    bool check_json(const json::value& json) const { return json.is_string(); }

    bool from_json(const json::value& json, std::wstring& wstr) const
    {
        wstr = MAA_NS::to_u16(json.as_string());
        return true;
    }
};

template <>
class jsonization<cv::Rect>
{
public:
    json::value to_json(const cv::Rect& rect) const
    {
        return json::array { rect.x, rect.y, rect.width, rect.height };
    }

    bool check_json(const json::value& json) const
    {
        return json.is<std::vector<int>>() && json.as_array().size() == 4;
    }

    bool from_json(const json::value& json, cv::Rect& rect) const
    {
        auto arr = json.as<std::vector<int>>();
        rect = cv::Rect(arr[0], arr[1], arr[2], arr[3]);
        return true;
    }
};
} // namespace json::ext