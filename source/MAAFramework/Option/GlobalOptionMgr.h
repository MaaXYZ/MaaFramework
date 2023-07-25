#pragma once

#include <filesystem>
#include <string_view>

#include "MaaConf.h"
#include "MaaDef.h"
#include "Utils/SingletonHolder.hpp"

MAA_NS_BEGIN

class GlobalOptionMgr : public SingletonHolder<GlobalOptionMgr>
{
public:
    friend class SingletonHolder<GlobalOptionMgr>;

public:
    virtual ~GlobalOptionMgr() = default;

    bool set_option(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size);

private:
    GlobalOptionMgr() = default;

private:
    bool set_logging(MaaOptionValue value, MaaOptionValueSize val_size);
};

MAA_NS_END
