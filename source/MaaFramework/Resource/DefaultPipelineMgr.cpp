#include "DefaultPipelineMgr.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/StringMisc.hpp"
#include "PipelineParser.h"
#include "Vision/VisionTypes.h"

MAA_RES_NS_BEGIN

bool DefaultPipelineMgr::load(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    auto json_opt = json::open(path, true, true);
    if (!json_opt) {
        LogError << "json::open failed" << VAR(path);
        return false;
    }
    const auto& json = *json_opt;
    LogInfo << VAR(json);

    return parse_pipeline(json) && parse_recognition(json) && parse_action(json);
}

void DefaultPipelineMgr::clear()
{
    LogFunc;

    recognition_param_.clear();
    action_param_.clear();
}

bool DefaultPipelineMgr::parse_pipeline(const json::value& input)
{
    static const std::string kDefaultKey = "Default";
    auto opt = input.find(kDefaultKey);
    if (!opt) {
        LogDebug << "no default key found in pipeline" << VAR(input);
        return true;
    }

    return PipelineParser::parse_node(std::string(), *opt, pipeline_param_, {}, {});
}

bool DefaultPipelineMgr::parse_recognition(const json::value& input)
{
    using namespace MAA_RES_NS::Recognition;

    for (const auto& [name, type] : kTypeMap) {
        auto opt = input.find(name);
        if (!opt) {
            continue;
        }

        Type parsed_type = Type::Invalid;
        Param parsed_param;
        bool ret = PipelineParser::parse_recognition(*opt, parsed_type, parsed_param, Type::Invalid, {}, {});
        if (!ret) {
            LogError << "parse_recognition failed" << VAR(name);
            return false;
        }
        if (parsed_type != type) {
            LogError << "parsed_type != type";
            return false;
        }
        add_recognition_param(parsed_type, std::move(parsed_param));
    }

    return true;
}

bool DefaultPipelineMgr::parse_action(const json::value& input)
{
    using namespace MAA_RES_NS::Action;

    for (const auto& [name, type] : kTypeMap) {
        auto opt = input.find(name);
        if (!opt) {
            continue;
        }

        Type parsed_type = Type::Invalid;
        Param parsed_param;
        bool ret = PipelineParser::parse_action(*opt, parsed_type, parsed_param, Type::Invalid, {}, {});
        if (!ret) {
            LogError << "parse_action failed" << VAR(name);
            return false;
        }
        if (parsed_type != type) {
            LogError << "parsed_type != type";
            return false;
        }
        add_action_param(parsed_type, std::move(parsed_param));
    }

    return true;
}

void DefaultPipelineMgr::add_recognition_param(Recognition::Type type, Recognition::Param param)
{
    recognition_param_.insert_or_assign(type, std::move(param));
}

void DefaultPipelineMgr::add_action_param(Action::Type type, Action::Param param)
{
    action_param_.insert_or_assign(type, std::move(param));
}

MAA_RES_NS_END
