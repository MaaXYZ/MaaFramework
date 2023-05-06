#pragma once

#include "MaaConf.h"
#include "MaaDef.h"
#include "MaaMsg.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#ifndef MAA_VERSION
#define MAA_VERSION "DEBUG_VERSION"
#endif

MAA_NS_BEGIN

static constexpr double DoubleDiff = 1e-12;
static constexpr std::string_view UploadDataSource = "MAA";

enum class StaticOptionKey
{
    Invalid = 0,
};

enum class ResourceOptionKey
{
    Invalid = 0,
};

enum class ControllerOptionKey
{
    Invalid = 0,
    ControlMode = 1,
    AdbLiteEnabled = 2,
    KillAdbOnExit = 3,
};

enum class ControlModeValue
{
    Invalid = 0,
    Adb = 1,
    Minitouch = 2,
    Maatouch = 3,
    MacPlayTools = 4,
};

enum class InstanceOptionKey
{
    Invalid = 0,
};

struct MaaResourceAPI
{
public:
    virtual ~MaaResourceAPI() = default;

    virtual MaaResourceAPI* create_ex(const std::filesystem::path& path, MaaResourceCallback callback,
                                      void* callback_arg) = 0;
    virtual void destroy(MaaResourceAPI** handle_ptr) = 0;

    virtual bool set_option(ResourceOptionKey key, const std::string& value) = 0;

    virtual bool incremental_load(const std::filesystem::path& path) = 0;
    virtual bool loading() const = 0;
    virtual bool loaded() const = 0;

    virtual std::string get_hash() const = 0;
};

struct MaaControllerAPI
{
public:
    virtual ~MaaControllerAPI() = default;

    virtual MaaControllerAPI* create_ex(const std::filesystem::path& adb_path, const std::string& address,
                                        const json::value& config_json, MaaControllerCallback callback,
                                        void* callback_arg) = 0;
    virtual void destroy(MaaControllerAPI** handle_ptr) = 0;

    virtual bool set_option(ControllerOptionKey key, const std::string& value) = 0;

    virtual bool connecting() const = 0;
    virtual bool connected() const = 0;

    virtual MaaCtrlId click(int x, int y) = 0;
    virtual MaaCtrlId screencap() = 0;
    virtual std::vector<unsigned char> get_image() const = 0;

    virtual std::string get_uuid() const = 0;
};

struct MaaInstanceAPI
{
public:
    virtual ~MaaInstanceAPI() = default;

    virtual MaaInstanceAPI* create_ex(const std::filesystem::path& user_path, MaaInstanceCallback callback,
                                      void* callback_arg) = 0;
    virtual void destroy(MaaInstanceAPI** handle_ptr) = 0;

    virtual bool bind_resource(MaaResourceAPI* resource) = 0;
    virtual bool bind_controller(MaaControllerAPI* controller) = 0;
    virtual bool inited() const = 0;

    virtual bool set_option(InstanceOptionKey key, const std::string& value) = 0;

    virtual MaaTaskId append_task(const std::string& type, const std::string& param) = 0;
    virtual bool set_task_param(MaaTaskId task_id, const std::string& param) = 0;
    virtual std::vector<MaaTaskId> get_task_list() const = 0;

    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool running() const = 0;

    virtual std::string get_resource_hash() const = 0;
    virtual std::string get_controller_uuid() const = 0;
};

MAA_NS_END
