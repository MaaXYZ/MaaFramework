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

#ifdef MAA_DLL_EXPORTS // defined if we are building the DLL (instead of using it)
#define MAA_API MAA_DLL_EXPORT
#else
#define MAA_API MAA_DLL_IMPORT
#endif // MAA_DLL_EXPORTS

#if defined MAA_DEBUG_DLL_EXPORTS
#define MAA_DEBUG_API MAA_DLL_EXPORT
#elif defined MAA_DLL_EXPORTS
#define MAA_DEBUG_API
#else
#define MAA_DEBUG_API MAA_DLL_IMPORT
#endif

#ifdef _WIN32
#pragma warning(disable : 4251)
#endif
