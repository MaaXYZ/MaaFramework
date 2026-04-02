#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include <boost/function.hpp>

#include "MaaUtils/LibraryHolder.h"

namespace AndroidNativeNS
{

// Frame info structure - for reading screen frames (32 bytes)
struct FrameInfo
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t stride = 0;
    uint32_t length = 0;
    void* data = nullptr;
    void* frame_ref = nullptr;
};

enum MethodType : int
{
    START_GAME = 1,
    STOP_GAME = 2,
    INPUT = 4,
    TOUCH_DOWN = 6,
    TOUCH_MOVE = 7,
    TOUCH_UP = 8,
    KEY_DOWN = 9,
    KEY_UP = 10
};

struct Position
{
    int x = 0;
    int y = 0;
};

struct StartGameArgs
{
    const char* package_name = nullptr;
    int force_stop = 0;
};

struct StopGameArgs
{
    const char* client_type = nullptr;
};

struct InputArgs
{
    const char* text = nullptr;
};

struct TouchArgs
{
    Position p { };
};

struct KeyArgs
{
    int key_code = 0;
};

union ArgUnion
{
    StartGameArgs start_game;
    StopGameArgs stop_game;
    InputArgs input;
    TouchArgs touch;
    KeyArgs key;
};

struct MethodParam
{
    int display_id = 0;
    MethodType method = START_GAME;
    ArgUnion args { };
};

using GetLockedPixelsSignature = FrameInfo();
using UnlockPixelsSignature = int(FrameInfo);
using DispatchInputMessageSignature = int(MethodParam);

using GetLockedPixelsFunc = boost::function<GetLockedPixelsSignature>;
using UnlockPixelsFunc = boost::function<UnlockPixelsSignature>;
using DispatchInputMessageFunc = boost::function<DispatchInputMessageSignature>;

struct AndroidExternalFunctions
{
    GetLockedPixelsFunc get_locked_pixels { };
    UnlockPixelsFunc unlock_pixels { };
    DispatchInputMessageFunc dispatch_input_message { };
};

} // namespace AndroidNativeNS

MAA_NS_BEGIN

class AndroidNativeExternalLibraryHolder : public LibraryHolder<AndroidNativeExternalLibraryHolder>
{
public:
    static std::optional<AndroidNativeNS::AndroidExternalFunctions> create_functions(const std::filesystem::path& library_path);

private:
    static constexpr auto get_locked_pixels_func_name_ = "GetLockedPixels";
    static constexpr auto unlock_pixels_func_name_ = "UnlockPixels";
    static constexpr auto dispatch_input_message_func_name_ = "DispatchInputMessage";
};

MAA_NS_END
