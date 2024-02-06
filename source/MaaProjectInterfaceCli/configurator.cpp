#include "configurator.h"

#include <ranges>

#include "ProjectInterface/Parser.h"

using namespace MAA_PROJECT_INTERFACE_NS;

bool Configurator::load(const std::filesystem::path& project_dir)
{
    auto data_opt = Parser::parse_interface(project_dir / kInterfaceFilename);
    if (!data_opt) {
        return false;
    }
    data_ = *data_opt;

    Configuration config;
    auto cfg_json_opt = json::open(project_dir / kConfigFilename);
    if (cfg_json_opt) {
        if (auto cfg_opt = Parser::parse_config(*cfg_json_opt)) {
            config = *cfg_opt;
        }
    }

    project_dir_ = project_dir;
    return true;
}

void Configurator::save()
{
    std::ofstream(project_dir_ / kInterfaceFilename) << config_.to_json();
}

void Configurator::check_config()
{
    auto resource_iter =
        std::ranges::find_if(data_.resource, [&](const auto& resource) { return resource.name == config_.resource; });

    if (resource_iter == data_.resource.end()) {
        select_resource();
    }
    else {
        runtime_.resource_path = resource_iter->path;
    }

    for (const auto& config_task : config_.task) {}

    runtime_.executor = data_.executor;
}

void Configurator::select_resource()
{
    Resource res;

    // TODO

    config_.resource = res.name;
    runtime_.resource_path = res.path;
}
