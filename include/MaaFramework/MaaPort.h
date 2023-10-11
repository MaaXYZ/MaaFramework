#pragma once

// The function exported symbols
#if defined _WIN32 || defined __CYGWIN__
#define MAA_DLL_IMPORT __declspec(dllimport)
#define MAA_DLL_EXPORT __declspec(dllexport)
#define MAA_DLL_LOCAL
#else
#if __GNUC__ >= 4
#define MAA_DLL_IMPORT __attribute__((visibility("default")))
#define MAA_DLL_EXPORT __attribute__((visibility("default")))
#define MAA_DLL_LOCAL __attribute__((visibility("hidden")))
#else
#define MAA_DLL_IMPORT
#define MAA_DLL_EXPORT
#define MAA_DLL_LOCAL
#endif
#endif

#ifdef MAA_FRAMEWORK_EXPORTS // defined if we are building the DLL (instead of using it)
#define MAA_FRAMEWORK_API MAA_DLL_EXPORT
#else
#define MAA_FRAMEWORK_API MAA_DLL_IMPORT
#endif // MAA_FRAMEWORK_EXPORTS

#ifdef MAA_CONTROL_UNIT_EXPORTS
#define MAA_CONTROL_UNIT_API MAA_DLL_EXPORT
#else
#define MAA_CONTROL_UNIT_API MAA_DLL_IMPORT
#endif

#ifdef MAA_UTILS_EXPORTS
#define MAA_UTILS_API MAA_DLL_EXPORT
#else
#define MAA_UTILS_API MAA_DLL_IMPORT
#endif

#ifdef MAA_TOOLKIT_EXPORTS
#define MAA_TOOLKIT_API MAA_DLL_EXPORT
#else
#define MAA_TOOLKIT_API MAA_DLL_IMPORT
#endif

#ifdef MAA_RPC_EXPORTS
#define MAA_RPC_API MAA_DLL_EXPORT
#else
#define MAA_RPC_API MAA_DLL_IMPORT
#endif
