#pragma once

#include "MaaFramework/MaaDef.h"
#include "MaaPI/MaaPIAPI.h"

#include <filesystem>
#include <functional>
#include <meojson/json.hpp>

class Interactor
{
public:
    Interactor();

    bool load(const std::filesystem::path& project_dir);
    void print_config() const;
    void interact();
    bool run();

private:
    void welcome() const;
    bool interact_once();

    void select_controller();
    void select_resource();
    void add_task();
    void edit_task();
    void delete_task();
    void move_task();

    void print_config_tasks(bool with_index = true) const;

    void mpause() const;
    bool save_config() const;

    static void on_maafw_notify(const char* msg, const char* details_json, void* callback_arg);
    // static std::string format_win32_config(const MAA_PROJECT_INTERFACE_NS::Configuration::Win32Config& win32_config);
    static int32_t on_maafw_client_handler(
        MaaPIClient* client,
        MaaPIClientAction action,
        MaaStringListBuffer* choice,
        MaaStringListBuffer* details,
        void* arg);

private:
    std::filesystem::path project_dir_;

    MaaPIData* data_;
    MaaPIConfig* config_;
    MaaPIRuntime* runtime_;
    MaaPIClient* client_;

    // MAA_PROJECT_INTERFACE_NS::Configurator config_;

    struct ClientHandleInfo
    {
        struct ChoiceInfo
        {
            std::string choice;
            json::object detail;
        };

        std::vector<ChoiceInfo> choices;
    };

    std::function<int32_t(const ClientHandleInfo& info)> client_handler_;
};
