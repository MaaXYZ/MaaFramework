#pragma once

// The function exported symbols
#if defined _WIN32 || defined __CYGWIN__
#define MAA_TOOLKIT_DLL_IMPORT __declspec(dllimport)
#define MAA_TOOLKIT_DLL_EXPORT __declspec(dllexport)
#define MAA_TOOLKIT_DLL_LOCAL
#else
#if __GNUC__ >= 4
#define MAA_TOOLKIT_DLL_IMPORT __attribute__((visibility("default")))
#define MAA_TOOLKIT_DLL_EXPORT __attribute__((visibility("default")))
#define MAA_TOOLKIT_DLL_LOCAL __attribute__((visibility("hidden")))
#else
#define MAA_TOOLKIT_DLL_IMPORT
#define MAA_TOOLKIT_DLL_EXPORT
#define MAA_TOOLKIT_DLL_LOCAL
#endif
#endif

#ifdef MAA_TOOLKIT_DLL_EXPORTS // defined if we are building the DLL (instead of using it)
#define MAA_TOOLKIT_API MAA_TOOLKIT_DLL_EXPORT
#else
#define MAA_TOOLKIT_API MAA_TOOLKIT_DLL_IMPORT
#endif // MAA_TOOLKIT_DLL_EXPORTS

#ifdef MAA_TOOLKIT_UTILS_EXPORTS
#define MAA_TOOLKIT_UTILS_API MAA_TOOLKIT_DLL_EXPORT
#else
#define MAA_TOOLKIT_UTILS_API MAA_TOOLKIT_DLL_IMPORT
#endif

#if defined MAA_TOOLKIT_DEBUG_DLL_EXPORTS
#define MAA_TOOLKIT_DEBUG_API MAA_TOOLKIT_DLL_EXPORT
#elif defined MAA_TOOLKIT_DLL_EXPORTS
#define MAA_TOOLKIT_DEBUG_API
#else
#define MAA_TOOLKIT_DEBUG_API MAA_TOOLKIT_DLL_IMPORT
#endif

#ifdef _WIN32
#pragma warning(disable : 4251)
#endif
