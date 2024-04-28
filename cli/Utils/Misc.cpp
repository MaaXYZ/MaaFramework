#include "Utils/Misc.h"

#define BOOST_DLL_USE_STD_FS
#include <boost/dll.hpp>

namespace maa::cli
{

std::filesystem::path program_directory()
{
    return boost::dll::program_location().parent_path();
}

}
