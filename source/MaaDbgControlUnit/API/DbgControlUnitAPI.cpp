#include "ControlUnit/DbgControlUnitAPI.h"

#include "Utils/Logger.h"

MAA_DBG_CTRL_UNIT_NS_BEGIN

std::shared_ptr<ControllerAPI> create_controller(MaaStringView read_path, MaaStringView write_path,
                                                 MaaDbgControllerType type, MaaStringView config)
{
    LogFunc << VAR(read_path) << VAR(write_path) << VAR(type) << VAR(config);

    return nullptr;
}

MAA_DBG_CTRL_UNIT_NS_END
