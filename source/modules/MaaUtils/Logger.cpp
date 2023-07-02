#include "MaaUtils/Logger.hpp"

MAA_NS_BEGIN

Logger& Logger::get_instance()
{
    static Logger unique_instance;
    return unique_instance;
}

MAA_NS_END
