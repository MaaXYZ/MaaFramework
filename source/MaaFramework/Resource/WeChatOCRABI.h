#pragma once

#include <cstdint>

// Independent declarations for the WeChatOCR SDK C ABI v1. No vendor runtime
// interfaces are used by MaaFramework; all symbols are resolved at runtime.
namespace wechat_ocr_abi
{
inline constexpr uint32_t kVersion = 1;
using Status = int32_t;
struct Engine;
struct Result;

struct Options
{
    uint32_t struct_size;
    uint32_t abi_version;
    const char* runtime_dir;
    const char* ocr_path;
    const char* worker_path;
    const char* temp_dir;
    uint32_t protocol;
    uint32_t startup_timeout_ms;
    uint32_t max_pending_requests;
};

struct Image
{
    uint32_t struct_size;
    uint32_t format;
    const uint8_t* data;
    uint64_t data_size;
    uint32_t width;
    uint32_t height;
    uint64_t stride;
};

struct Line
{
    uint32_t struct_size;
    uint32_t has_confidence;
    float quad[8];
    float confidence;
    const char* text;
    uint64_t text_length;
};

struct Capabilities
{
    uint32_t struct_size;
    uint32_t abi_version;
    uint32_t detect_and_recognize;
    uint32_t recognize_only;
    uint32_t native_batch;
    uint32_t native_memory_input;
};

struct ResultInfo
{
    uint32_t struct_size;
    uint32_t width;
    uint32_t height;
    int32_t vendor_error;
};

#if INTPTR_MAX == INT64_MAX
static_assert(sizeof(Options) == 56);
static_assert(sizeof(Image) == 40);
static_assert(sizeof(Line) == 64);
static_assert(sizeof(Capabilities) == 24);
static_assert(sizeof(ResultInfo) == 16);
#endif

#ifdef _WIN32
#define MAA_WCO_CALL __cdecl
#else
#define MAA_WCO_CALL
#endif
using Version = const char*(MAA_WCO_CALL*)();
using LastError = const char*(MAA_WCO_CALL*)();
using Create = Status(MAA_WCO_CALL*)(const Options*, Engine**);
using Recognize = Status(MAA_WCO_CALL*)(Engine*, const Image*, uint32_t, Result**);
using GetCapabilities = Status(MAA_WCO_CALL*)(const Engine*, Capabilities*);
using LineCount = uint32_t(MAA_WCO_CALL*)(const Result*);
using GetInfo = Status(MAA_WCO_CALL*)(const Result*, ResultInfo*);
using GetLine = Status(MAA_WCO_CALL*)(const Result*, uint32_t, Line*);
using ResultDestroy = void(MAA_WCO_CALL*)(Result*);
using Destroy = void(MAA_WCO_CALL*)(Engine*);
#undef MAA_WCO_CALL
}
