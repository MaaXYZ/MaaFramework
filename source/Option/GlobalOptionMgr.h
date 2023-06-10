#pragma once

#include <filesystem>
#include <string_view>

#include "Base/SingletonHolder.hpp"
#include "Common/MaaConf.h"
#include "MaaDef.h"

MAA_NS_BEGIN

class GlobalOptionMgr : public SingletonHolder<GlobalOptionMgr>
{
public:
    friend class SingletonHolder<GlobalOptionMgr>;

public:
    virtual ~GlobalOptionMgr() = default;

    bool set_option(MaaGlobalOption key, const std::string& value);

protected:
    GlobalOptionMgr() = default;
};

MAA_NS_END
