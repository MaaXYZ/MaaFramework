#pragma once

#include "Conf/Conf.h"

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Win7
#endif
#endif

#define BOOST_PROCESS_USE_STD_FS 1

MAA_SUPPRESS_BOOST_WARNINGS_BEGIN
#include <boost/asio.hpp>
#include <boost/process.hpp>
#ifdef _WIN32
#include <boost/process/windows.hpp>
#endif
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
MAA_SUPPRESS_BOOST_WARNINGS_END
