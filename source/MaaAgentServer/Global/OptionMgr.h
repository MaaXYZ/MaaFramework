#pragma once

#include "Common/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "MaaUtils/SingletonHolder.hpp"

MAA_AGENT_SERVER_NS_BEGIN

class OptionMgr : public SingletonHolder<OptionMgr>
{
public:
    friend class SingletonHolder<OptionMgr>;

public:
    virtual ~OptionMgr() = default;
    bool set_option(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size);

private:
    OptionMgr() = default;

private:
    bool set_log_dir(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_stdout_level(MaaOptionValue value, MaaOptionValueSize val_size);
};

MAA_AGENT_SERVER_NS_END
