#pragma once

#include <unordered_map>
#include <vector>

#include "Common/MaaConf.h"
#include "Platform/Platform.h"

MAA_NS_BEGIN

struct ArgVItem
{
    enum Type
    {
        RAW,
        KEY
    } type_;
    std::string key_;

    ArgVItem(std::string key, Type type = Type::RAW) : type_(type), key_(key) {}
};

class ArgV;

struct ArgVInst
{
    using os_string = MAA_PLATFORM_NS::os_string;

    ArgVInst(const ArgV* a, const std::unordered_map<std::string, std::string>& p) : argv(a), pat(p) {}

    std::vector<os_string> getArgV() const;
    os_string getCommandLine() const;

    const ArgV* argv;
    const std::unordered_map<std::string, std::string>& pat;
};

class ArgV
{
public:
    using os_string = MAA_PLATFORM_NS::os_string;

    ArgV() = default;

    ArgV& raw(std::string arg)
    {
        args.emplace_back(arg);
        return *this;
    }

    ArgV& key(std::string arg)
    {
        args.emplace_back(arg, ArgVItem::KEY);
        return *this;
    }

    std::vector<os_string> getArgV(const std::unordered_map<std::string, std::string>& pat) const
    {
        std::vector<os_string> result;
        for (auto item : args) {
            switch (item.type_) {
            case ArgVItem::RAW:
                result.emplace_back(MAA_PLATFORM_NS::to_osstring(item.key_));
                break;
            case ArgVItem::KEY:
                result.emplace_back(MAA_PLATFORM_NS::to_osstring(
                    pat.contains(item.key_) ? pat.at(item.key_) : "[ERROR: " + item.key_ + " not found]"));
                break;
            }
        }
        return result;
    }

    os_string getCommandLine(const std::unordered_map<std::string, std::string>& pat) const
    {
        return MAA_PLATFORM_NS::args_to_cmd(getArgV(pat));
    }

    ArgVInst getInstance(const std::unordered_map<std::string, std::string>& pat) const { return ArgVInst(this, pat); }

private:
    std::vector<ArgVItem> args;
};


inline std::vector<ArgVInst::os_string> ArgVInst::getArgV() const
{
    return argv->getArgV(pat);
}

ArgVInst::os_string ArgVInst::getCommandLine() const
{
    return argv->getCommandLine(pat);
}

MAA_NS_END
