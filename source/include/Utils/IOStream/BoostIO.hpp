#pragma once

#define BOOST_PROCESS_USE_STD_FS 1

#include <boost/asio.hpp>
#include <boost/process.hpp>
#ifdef _WIN32
#include <boost/process/extend.hpp>
#include <boost/process/windows.hpp>
#endif
