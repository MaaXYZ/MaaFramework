#include "DefaultPipelineMgr.h"

#include "PipelineResMgr.h"
#include "Utils/Codec.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/StringMisc.hpp"
#include "Vision/VisionTypes.h"

MAA_RES_NS_BEGIN

bool DefaultPipelineMgr::load(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    auto json_opt = json::open(path);
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

    return PipelineResMgr::parse_node(std::string(), *opt, pipeline_param_, {}, {});
}

bool DefaultPipelineMgr::parse_recognition(const json::value& input)
{
    using namespace MAA_RES_NS::Recognition;

    static const std::unordered_map<std::string, Type> kRecTypeMap = {
        { "DirectHit", Type::DirectHit },
        { "directhit", Type::DirectHit },
        { "TemplateMatch", Type::TemplateMatch },
        { "templatematch", Type::TemplateMatch },
        { "FeatureMatch", Type::FeatureMatch },
        { "featurematch", Type::FeatureMatch },
        { "ColorMatch", Type::ColorMatch },
        { "colormatch", Type::ColorMatch },
        { "OCR", Type::OCR },
        { "ocr", Type::OCR },
        { "NeuralNetworkClassify", Type::NeuralNetworkClassify },
        { "neuralnetworkclassify", Type::NeuralNetworkClassify },
        { "nnclassify", Type::NeuralNetworkClassify },
        { "NNClassify", Type::NeuralNetworkClassify },
        { "NeuralNetworkDetect", Type::NeuralNetworkDetect },
        { "neuralnetworkdetect", Type::NeuralNetworkDetect },
        { "NNDetect", Type::NeuralNetworkDetect },
        { "nnDetect", Type::NeuralNetworkDetect },
        { "Custom", Type::Custom },
        { "custom", Type::Custom },
    };

    for (const auto& [name, type] : kRecTypeMap) {
        auto opt = input.find(name);
        if (!opt) {
            continue;
        }

        Type parsed_type = Type::Invalid;
        Param parsed_param;
        bool ret = PipelineResMgr::parse_recognition(*opt, parsed_type, parsed_param, Type::Invalid, {}, {});
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

    const std::unordered_map<std::string, Type> kActTypeMap = {
        { "DoNothing", Type::DoNothing },
        { "donothing", Type::DoNothing },
        { "Click", Type::Click },
        { "click", Type::Click },
        { "Swipe", Type::Swipe },
        { "swipe", Type::Swipe },
        { "MultiSwipe", Type::MultiSwipe },
        { "multiswipe", Type::MultiSwipe },
        { "PressKey", Type::Key },
        { "presskey", Type::Key },
        { "Key", Type::Key },
        { "key", Type::Key },
        { "InputText", Type::Text },
        { "inputtext", Type::Text },
        { "Text", Type::Text },
        { "text", Type::Text },
        { "StartApp", Type::StartApp },
        { "startapp", Type::StartApp },
        { "StopApp", Type::StopApp },
        { "stopapp", Type::StopApp },
        { "Command", Type::Command },
        { "command", Type::Command },
        { "Custom", Type::Custom },
        { "custom", Type::Custom },
        { "StopTask", Type::StopTask },
        { "stoptask", Type::StopTask },
        { "Stop", Type::StopTask },
        { "stop", Type::StopTask },
    };

    for (const auto& [name, type] : kActTypeMap) {
        auto opt = input.find(name);
        if (!opt) {
            continue;
        }

        Type parsed_type = Type::Invalid;
        Param parsed_param;
        bool ret = PipelineResMgr::parse_action(*opt, parsed_type, parsed_param, Type::Invalid, {}, {});
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
