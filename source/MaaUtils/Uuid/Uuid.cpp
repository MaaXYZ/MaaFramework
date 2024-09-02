#include "Utils/Uuid.h"

MAA_SUPPRESS_BOOST_WARNINGS_BEGIN
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
MAA_SUPPRESS_BOOST_WARNINGS_END

MAA_NS_BEGIN

std::string make_uuid()
{
    static boost::uuids::random_generator uuid_generator;
    return boost::uuids::to_string(uuid_generator());
}

MAA_NS_END
