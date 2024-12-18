#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "Conf/Conf.h"
#include "Resource/PipelineTypes.h"
#include "Utils/Platform.h"

MAA_TASK_NS_BEGIN

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
    ~CommandAction();

    bool run(const MAA_RES_NS::Action::CommandParam& command, const Runtime& runtime);

private:
    std::string get_entry_name(const Runtime& runtime);
    std::string get_node_name(const Runtime& runtime);
    std::string get_image_path(const Runtime& runtime);
    std::string get_box(const Runtime& runtime);
    std::string get_library_dir(const Runtime& runtime);
    std::string get_resource_dir(const Runtime& runtime);

private:
    static std::vector<std::filesystem::path> cached_images_;

    std::string image_path_;
};

MAA_TASK_NS_END
