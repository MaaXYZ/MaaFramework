#pragma once

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00 // Win10
#endif
#endif

#define BOOST_PROCESS_USE_STD_FS 1

#include <boost/asio.hpp>
#include <boost/process.hpp>
#ifdef _WIN32
#include <boost/process/extend.hpp>
#include <boost/process/windows.hpp>
#endif