#pragma once

// The function exported symbols
#if defined _WIN32 || defined __CYGWIN__
#define MAA_PLUGIN_DLL_IMPORT __declspec(dllimport)
#define MAA_PLUGIN_DLL_EXPORT __declspec(dllexport)
#define MAA_PLUGIN_DLL_LOCAL
#else
#if __GNUC__ >= 4
#define MAA_PLUGIN_DLL_IMPORT __attribute__((visibility("default")))
#define MAA_PLUGIN_DLL_EXPORT __attribute__((visibility("default")))
#define MAA_PLUGIN_DLL_LOCAL __attribute__((visibility("hidden")))
#else
#define MAA_PLUGIN_DLL_IMPORT
#define MAA_PLUGIN_DLL_EXPORT
#define MAA_PLUGIN_DLL_LOCAL
#endif
#endif

#ifdef MAA_PLUGIN_EXPORTS // defined if we are building the DLL (instead of using it)
#define MAA_PLUGIN_API MAA_PLUGIN_DLL_EXPORT
#else
#define MAA_PLUGIN_API MAA_PLUGIN_DLL_IMPORT
#endif // MAA_PLUGIN_EXPORTS
