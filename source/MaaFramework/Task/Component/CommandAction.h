#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "Common/Conf.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/SingletonHolder.hpp"
#include "Resource/PipelineTypes.h"

MAA_TASK_NS_BEGIN

class TempFileHolder : public SingletonHolder<TempFileHolder>
{
public:
    virtual ~TempFileHolder() override;

    void emplace(const std::filesystem::path& p);

private:
    std::vector<std::filesystem::path> files_;
};

class CommandAction
{
public:
    struct Runtime
    {
        std::vector<std::filesystem::path> resource_paths;

        std::string entry;
        std::string node;
        cv::Mat image;
        cv::Rect box {};
    };

public:
    bool run(const MAA_RES_NS::Action::CommandParam& command, const Runtime& runtime);

private:
    std::string get_entry_name(const Runtime& runtime);
    std::string get_node_name(const Runtime& runtime);
    std::string get_image_path(const Runtime& runtime);
    std::string get_box(const Runtime& runtime);
    std::string get_library_dir(const Runtime& runtime);
    std::string get_resource_dir(const Runtime& runtime);

private:
    std::string image_path_;
};

MAA_TASK_NS_END
