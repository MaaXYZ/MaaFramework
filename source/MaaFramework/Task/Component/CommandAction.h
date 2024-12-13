#pragma once

#include <string>
#include <vector>

#include "Conf/Conf.h"
#include "Resource/PipelineTypes.h"
#include "Utils/SingletonHolder.hpp"
#include "Utils/Platform.h"

MAA_TASK_NS_BEGIN

class CommandAction : public SingletonHolder<CommandAction>
{
public:
    struct Runtime
    {
        std::string entry;
        std::string node;
        cv::Mat image;
        cv::Rect box {};
    };

public:
    ~CommandAction();

    bool run(const MAA_RES_NS::Action::CommandParam& command, const Runtime& runtime);

private:
    std::string gen_entry_name(const Runtime& runtime);
    std::string gen_node_name(const Runtime& runtime);
    std::string gen_image_path(const Runtime& runtime);
    std::string gen_box(const Runtime& runtime);

private:
    std::vector<std::filesystem::path> cached_images_;
};

MAA_TASK_NS_END
