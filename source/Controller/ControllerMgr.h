#pragma once

#include "Common/MaaMsg.h"
#include "Common/MaaTypes.h"

MAA_CTRL_NS_BEGIN

class ControllerMgr : public MaaControllerAPI
{
public:
    struct Config
    {};

public:
    virtual ~ControllerMgr();

    virtual MaaControllerAPI* create_ex(const std::filesystem::path& adb_path, const std::string& address,
                                        const json::value& config_json, MaaControllerCallback callback,
                                        void* callback_arg) override;
    virtual void destroy(MaaControllerAPI** handle_ptr) override;

    virtual bool set_option(ControllerOptionKey key, const std::string& value) override;

    virtual bool connecting() const override;
    virtual bool connected() const override;

    virtual MaaCtrlId click(int x, int y) override;
    virtual MaaCtrlId screencap() override;
    virtual std::vector<unsigned char> get_image() const override;

    virtual std::string get_uuid() const override;

protected:
    ControllerMgr(const std::filesystem::path& adb_path, const std::string& address, const Config& config,
                  MaaControllerCallback callback, void* callback_arg);

protected:
    std::filesystem::path adb_path_;
    std::string address_;
    Config config_;
    MaaControllerCallback callback_ = nullptr;
    void* callback_arg_ = nullptr;
};

MAA_CTRL_NS_END
