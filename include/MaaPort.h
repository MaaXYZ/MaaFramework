#pragma once

// The way how the function is called
#if !defined(MAA_CALL)
#if defined(_WIN32)
#define MAA_CALL __stdcall
#else
#define MAA_CALL
#endif /* _WIN32 */
#endif /* MAA_CALL */

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
#define MAAAPI_PORT MAA_DLL_EXPORT
#else
#define MAAAPI_PORT MAA_DLL_IMPORT
#endif // MAA_DLL_EXPORTS

#define MAAAPI MAAAPI_PORT MAA_CALL

#define MAALOCAL MAA_DLL_LOCAL MAA_CALL
