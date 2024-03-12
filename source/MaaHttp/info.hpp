// clang-format off
#pragma once

#include "function/interface.hpp"
#include "callback/interface.hpp"

#include "include.h"

namespace lhg::maa {

struct func_type_MaaAdbControllerCreate {
  struct _0_adb_path {
    constexpr static size_t index = 0;
    constexpr static const char* name = "adb_path";
    using type = MaaStringView;
  };
  struct _1_address {
    constexpr static size_t index = 1;
    constexpr static const char* name = "address";
    using type = MaaStringView;
  };
  struct _2_type {
    constexpr static size_t index = 2;
    constexpr static const char* name = "type";
    using type = MaaAdbControllerType;
  };
  struct _3_config {
    constexpr static size_t index = 3;
    constexpr static const char* name = "config";
    using type = MaaStringView;
  };
  struct _4_callback {
    constexpr static size_t index = 4;
    constexpr static const char* name = "callback";
    using type = MaaControllerCallback;
  };
  struct _5_callback_arg {
    constexpr static size_t index = 5;
    constexpr static const char* name = "callback_arg";
    using type = MaaCallbackTransparentArg;
  };
  struct ret {
    constexpr static size_t index = 6;
    constexpr static const char* name = "return";
    using type = MaaControllerHandle;
  };
  using args = std::tuple<_0_adb_path, _1_address, _2_type, _3_config, _4_callback, _5_callback_arg, ret>;
};
struct function_MaaAdbControllerCreate {
  constexpr static auto func = MaaAdbControllerCreate;
  constexpr static const char* name = "MaaAdbControllerCreate";
  using type = func_type_MaaAdbControllerCreate;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaAdbControllerCreate::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaAdbControllerCreate::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaWin32ControllerCreate {
  struct _0_hWnd {
    constexpr static size_t index = 0;
    constexpr static const char* name = "hWnd";
    using type = MaaWin32Hwnd;
  };
  struct _1_type {
    constexpr static size_t index = 1;
    constexpr static const char* name = "type";
    using type = MaaWin32ControllerType;
  };
  struct _2_callback {
    constexpr static size_t index = 2;
    constexpr static const char* name = "callback";
    using type = MaaControllerCallback;
  };
  struct _3_callback_arg {
    constexpr static size_t index = 3;
    constexpr static const char* name = "callback_arg";
    using type = MaaCallbackTransparentArg;
  };
  struct ret {
    constexpr static size_t index = 4;
    constexpr static const char* name = "return";
    using type = MaaControllerHandle;
  };
  using args = std::tuple<_0_hWnd, _1_type, _2_callback, _3_callback_arg, ret>;
};
struct function_MaaWin32ControllerCreate {
  constexpr static auto func = MaaWin32ControllerCreate;
  constexpr static const char* name = "MaaWin32ControllerCreate";
  using type = func_type_MaaWin32ControllerCreate;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaWin32ControllerCreate::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaWin32ControllerCreate::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaAdbControllerCreateV2 {
  struct _0_adb_path {
    constexpr static size_t index = 0;
    constexpr static const char* name = "adb_path";
    using type = MaaStringView;
  };
  struct _1_address {
    constexpr static size_t index = 1;
    constexpr static const char* name = "address";
    using type = MaaStringView;
  };
  struct _2_type {
    constexpr static size_t index = 2;
    constexpr static const char* name = "type";
    using type = MaaAdbControllerType;
  };
  struct _3_config {
    constexpr static size_t index = 3;
    constexpr static const char* name = "config";
    using type = MaaStringView;
  };
  struct _4_agent_path {
    constexpr static size_t index = 4;
    constexpr static const char* name = "agent_path";
    using type = MaaStringView;
  };
  struct _5_callback {
    constexpr static size_t index = 5;
    constexpr static const char* name = "callback";
    using type = MaaControllerCallback;
  };
  struct _6_callback_arg {
    constexpr static size_t index = 6;
    constexpr static const char* name = "callback_arg";
    using type = MaaCallbackTransparentArg;
  };
  struct ret {
    constexpr static size_t index = 7;
    constexpr static const char* name = "return";
    using type = MaaControllerHandle;
  };
  using args = std::tuple<_0_adb_path, _1_address, _2_type, _3_config, _4_agent_path, _5_callback, _6_callback_arg, ret>;
};
struct function_MaaAdbControllerCreateV2 {
  constexpr static auto func = MaaAdbControllerCreateV2;
  constexpr static const char* name = "MaaAdbControllerCreateV2";
  using type = func_type_MaaAdbControllerCreateV2;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaAdbControllerCreateV2::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaAdbControllerCreateV2::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaThriftControllerCreate {
  struct _0_type {
    constexpr static size_t index = 0;
    constexpr static const char* name = "type";
    using type = MaaThriftControllerType;
  };
  struct _1_host {
    constexpr static size_t index = 1;
    constexpr static const char* name = "host";
    using type = MaaStringView;
  };
  struct _2_port {
    constexpr static size_t index = 2;
    constexpr static const char* name = "port";
    using type = int32_t;
  };
  struct _3_config {
    constexpr static size_t index = 3;
    constexpr static const char* name = "config";
    using type = MaaStringView;
  };
  struct _4_callback {
    constexpr static size_t index = 4;
    constexpr static const char* name = "callback";
    using type = MaaControllerCallback;
  };
  struct _5_callback_arg {
    constexpr static size_t index = 5;
    constexpr static const char* name = "callback_arg";
    using type = MaaCallbackTransparentArg;
  };
  struct ret {
    constexpr static size_t index = 6;
    constexpr static const char* name = "return";
    using type = MaaControllerHandle;
  };
  using args = std::tuple<_0_type, _1_host, _2_port, _3_config, _4_callback, _5_callback_arg, ret>;
};
struct function_MaaThriftControllerCreate {
  constexpr static auto func = MaaThriftControllerCreate;
  constexpr static const char* name = "MaaThriftControllerCreate";
  using type = func_type_MaaThriftControllerCreate;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaThriftControllerCreate::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaThriftControllerCreate::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaDbgControllerCreate {
  struct _0_read_path {
    constexpr static size_t index = 0;
    constexpr static const char* name = "read_path";
    using type = MaaStringView;
  };
  struct _1_write_path {
    constexpr static size_t index = 1;
    constexpr static const char* name = "write_path";
    using type = MaaStringView;
  };
  struct _2_type {
    constexpr static size_t index = 2;
    constexpr static const char* name = "type";
    using type = MaaDbgControllerType;
  };
  struct _3_config {
    constexpr static size_t index = 3;
    constexpr static const char* name = "config";
    using type = MaaStringView;
  };
  struct _4_callback {
    constexpr static size_t index = 4;
    constexpr static const char* name = "callback";
    using type = MaaControllerCallback;
  };
  struct _5_callback_arg {
    constexpr static size_t index = 5;
    constexpr static const char* name = "callback_arg";
    using type = MaaCallbackTransparentArg;
  };
  struct ret {
    constexpr static size_t index = 6;
    constexpr static const char* name = "return";
    using type = MaaControllerHandle;
  };
  using args = std::tuple<_0_read_path, _1_write_path, _2_type, _3_config, _4_callback, _5_callback_arg, ret>;
};
struct function_MaaDbgControllerCreate {
  constexpr static auto func = MaaDbgControllerCreate;
  constexpr static const char* name = "MaaDbgControllerCreate";
  using type = func_type_MaaDbgControllerCreate;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaDbgControllerCreate::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaDbgControllerCreate::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerDestroy {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = void;
  };
  using args = std::tuple<_0_ctrl, ret>;
};
struct function_MaaControllerDestroy {
  constexpr static auto func = MaaControllerDestroy;
  constexpr static const char* name = "MaaControllerDestroy";
  using type = func_type_MaaControllerDestroy;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerDestroy::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerDestroy::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerPostConnection {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaCtrlId;
  };
  using args = std::tuple<_0_ctrl, ret>;
};
struct function_MaaControllerPostConnection {
  constexpr static auto func = MaaControllerPostConnection;
  constexpr static const char* name = "MaaControllerPostConnection";
  using type = func_type_MaaControllerPostConnection;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerPostConnection::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerPostConnection::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerPostClick {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct _1_x {
    constexpr static size_t index = 1;
    constexpr static const char* name = "x";
    using type = int32_t;
  };
  struct _2_y {
    constexpr static size_t index = 2;
    constexpr static const char* name = "y";
    using type = int32_t;
  };
  struct ret {
    constexpr static size_t index = 3;
    constexpr static const char* name = "return";
    using type = MaaCtrlId;
  };
  using args = std::tuple<_0_ctrl, _1_x, _2_y, ret>;
};
struct function_MaaControllerPostClick {
  constexpr static auto func = MaaControllerPostClick;
  constexpr static const char* name = "MaaControllerPostClick";
  using type = func_type_MaaControllerPostClick;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerPostClick::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerPostClick::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerPostSwipe {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct _1_x1 {
    constexpr static size_t index = 1;
    constexpr static const char* name = "x1";
    using type = int32_t;
  };
  struct _2_y1 {
    constexpr static size_t index = 2;
    constexpr static const char* name = "y1";
    using type = int32_t;
  };
  struct _3_x2 {
    constexpr static size_t index = 3;
    constexpr static const char* name = "x2";
    using type = int32_t;
  };
  struct _4_y2 {
    constexpr static size_t index = 4;
    constexpr static const char* name = "y2";
    using type = int32_t;
  };
  struct _5_duration {
    constexpr static size_t index = 5;
    constexpr static const char* name = "duration";
    using type = int32_t;
  };
  struct ret {
    constexpr static size_t index = 6;
    constexpr static const char* name = "return";
    using type = MaaCtrlId;
  };
  using args = std::tuple<_0_ctrl, _1_x1, _2_y1, _3_x2, _4_y2, _5_duration, ret>;
};
struct function_MaaControllerPostSwipe {
  constexpr static auto func = MaaControllerPostSwipe;
  constexpr static const char* name = "MaaControllerPostSwipe";
  using type = func_type_MaaControllerPostSwipe;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerPostSwipe::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerPostSwipe::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerPostPressKey {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct _1_keycode {
    constexpr static size_t index = 1;
    constexpr static const char* name = "keycode";
    using type = int32_t;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaCtrlId;
  };
  using args = std::tuple<_0_ctrl, _1_keycode, ret>;
};
struct function_MaaControllerPostPressKey {
  constexpr static auto func = MaaControllerPostPressKey;
  constexpr static const char* name = "MaaControllerPostPressKey";
  using type = func_type_MaaControllerPostPressKey;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerPostPressKey::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerPostPressKey::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerPostInputText {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct _1_text {
    constexpr static size_t index = 1;
    constexpr static const char* name = "text";
    using type = MaaStringView;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaCtrlId;
  };
  using args = std::tuple<_0_ctrl, _1_text, ret>;
};
struct function_MaaControllerPostInputText {
  constexpr static auto func = MaaControllerPostInputText;
  constexpr static const char* name = "MaaControllerPostInputText";
  using type = func_type_MaaControllerPostInputText;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerPostInputText::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerPostInputText::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerPostTouchDown {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct _1_contact {
    constexpr static size_t index = 1;
    constexpr static const char* name = "contact";
    using type = int32_t;
  };
  struct _2_x {
    constexpr static size_t index = 2;
    constexpr static const char* name = "x";
    using type = int32_t;
  };
  struct _3_y {
    constexpr static size_t index = 3;
    constexpr static const char* name = "y";
    using type = int32_t;
  };
  struct _4_pressure {
    constexpr static size_t index = 4;
    constexpr static const char* name = "pressure";
    using type = int32_t;
  };
  struct ret {
    constexpr static size_t index = 5;
    constexpr static const char* name = "return";
    using type = MaaCtrlId;
  };
  using args = std::tuple<_0_ctrl, _1_contact, _2_x, _3_y, _4_pressure, ret>;
};
struct function_MaaControllerPostTouchDown {
  constexpr static auto func = MaaControllerPostTouchDown;
  constexpr static const char* name = "MaaControllerPostTouchDown";
  using type = func_type_MaaControllerPostTouchDown;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerPostTouchDown::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerPostTouchDown::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerPostTouchMove {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct _1_contact {
    constexpr static size_t index = 1;
    constexpr static const char* name = "contact";
    using type = int32_t;
  };
  struct _2_x {
    constexpr static size_t index = 2;
    constexpr static const char* name = "x";
    using type = int32_t;
  };
  struct _3_y {
    constexpr static size_t index = 3;
    constexpr static const char* name = "y";
    using type = int32_t;
  };
  struct _4_pressure {
    constexpr static size_t index = 4;
    constexpr static const char* name = "pressure";
    using type = int32_t;
  };
  struct ret {
    constexpr static size_t index = 5;
    constexpr static const char* name = "return";
    using type = MaaCtrlId;
  };
  using args = std::tuple<_0_ctrl, _1_contact, _2_x, _3_y, _4_pressure, ret>;
};
struct function_MaaControllerPostTouchMove {
  constexpr static auto func = MaaControllerPostTouchMove;
  constexpr static const char* name = "MaaControllerPostTouchMove";
  using type = func_type_MaaControllerPostTouchMove;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerPostTouchMove::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerPostTouchMove::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerPostTouchUp {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct _1_contact {
    constexpr static size_t index = 1;
    constexpr static const char* name = "contact";
    using type = int32_t;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaCtrlId;
  };
  using args = std::tuple<_0_ctrl, _1_contact, ret>;
};
struct function_MaaControllerPostTouchUp {
  constexpr static auto func = MaaControllerPostTouchUp;
  constexpr static const char* name = "MaaControllerPostTouchUp";
  using type = func_type_MaaControllerPostTouchUp;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerPostTouchUp::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerPostTouchUp::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerPostScreencap {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaCtrlId;
  };
  using args = std::tuple<_0_ctrl, ret>;
};
struct function_MaaControllerPostScreencap {
  constexpr static auto func = MaaControllerPostScreencap;
  constexpr static const char* name = "MaaControllerPostScreencap";
  using type = func_type_MaaControllerPostScreencap;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerPostScreencap::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerPostScreencap::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerStatus {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct _1_id {
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = MaaCtrlId;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaStatus;
  };
  using args = std::tuple<_0_ctrl, _1_id, ret>;
};
struct function_MaaControllerStatus {
  constexpr static auto func = MaaControllerStatus;
  constexpr static const char* name = "MaaControllerStatus";
  using type = func_type_MaaControllerStatus;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerStatus::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerStatus::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerWait {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct _1_id {
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = MaaCtrlId;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaStatus;
  };
  using args = std::tuple<_0_ctrl, _1_id, ret>;
};
struct function_MaaControllerWait {
  constexpr static auto func = MaaControllerWait;
  constexpr static const char* name = "MaaControllerWait";
  using type = func_type_MaaControllerWait;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerWait::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerWait::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerConnected {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_ctrl, ret>;
};
struct function_MaaControllerConnected {
  constexpr static auto func = MaaControllerConnected;
  constexpr static const char* name = "MaaControllerConnected";
  using type = func_type_MaaControllerConnected;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerConnected::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerConnected::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerGetImage {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct _1_buffer {
    constexpr static size_t index = 1;
    constexpr static const char* name = "buffer";
    using type = MaaImageBufferHandle;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_ctrl, _1_buffer, ret>;
};
struct function_MaaControllerGetImage {
  constexpr static auto func = MaaControllerGetImage;
  constexpr static const char* name = "MaaControllerGetImage";
  using type = func_type_MaaControllerGetImage;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerGetImage::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerGetImage::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerGetUUID {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct _1_buffer {
    constexpr static size_t index = 1;
    constexpr static const char* name = "buffer";
    using type = MaaStringBufferHandle;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_ctrl, _1_buffer, ret>;
};
struct function_MaaControllerGetUUID {
  constexpr static auto func = MaaControllerGetUUID;
  constexpr static const char* name = "MaaControllerGetUUID";
  using type = func_type_MaaControllerGetUUID;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerGetUUID::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerGetUUID::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaCreate {
  struct _0_callback {
    constexpr static size_t index = 0;
    constexpr static const char* name = "callback";
    using type = MaaInstanceCallback;
  };
  struct _1_callback_arg {
    constexpr static size_t index = 1;
    constexpr static const char* name = "callback_arg";
    using type = MaaCallbackTransparentArg;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaInstanceHandle;
  };
  using args = std::tuple<_0_callback, _1_callback_arg, ret>;
};
struct function_MaaCreate {
  constexpr static auto func = MaaCreate;
  constexpr static const char* name = "MaaCreate";
  using type = func_type_MaaCreate;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaCreate::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaCreate::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaDestroy {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = void;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaDestroy {
  constexpr static auto func = MaaDestroy;
  constexpr static const char* name = "MaaDestroy";
  using type = func_type_MaaDestroy;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaDestroy::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaDestroy::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaBindResource {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct _1_res {
    constexpr static size_t index = 1;
    constexpr static const char* name = "res";
    using type = MaaResourceHandle;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_inst, _1_res, ret>;
};
struct function_MaaBindResource {
  constexpr static auto func = MaaBindResource;
  constexpr static const char* name = "MaaBindResource";
  using type = func_type_MaaBindResource;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaBindResource::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaBindResource::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaBindController {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct _1_ctrl {
    constexpr static size_t index = 1;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_inst, _1_ctrl, ret>;
};
struct function_MaaBindController {
  constexpr static auto func = MaaBindController;
  constexpr static const char* name = "MaaBindController";
  using type = func_type_MaaBindController;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaBindController::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaBindController::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaInited {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaInited {
  constexpr static auto func = MaaInited;
  constexpr static const char* name = "MaaInited";
  using type = func_type_MaaInited;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaInited::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaInited::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaRegisterCustomRecognizer {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct _1_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "name";
    using type = MaaStringView;
  };
  struct _2_recognizer {
    constexpr static size_t index = 2;
    constexpr static const char* name = "recognizer";
    using type = MaaCustomRecognizerHandle;
  };
  struct _3_recognizer_arg {
    constexpr static size_t index = 3;
    constexpr static const char* name = "recognizer_arg";
    using type = MaaTransparentArg;
  };
  struct ret {
    constexpr static size_t index = 4;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_inst, _1_name, _2_recognizer, _3_recognizer_arg, ret>;
};
struct function_MaaRegisterCustomRecognizer {
  constexpr static auto func = MaaRegisterCustomRecognizer;
  constexpr static const char* name = "MaaRegisterCustomRecognizer";
  using type = func_type_MaaRegisterCustomRecognizer;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaRegisterCustomRecognizer::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaRegisterCustomRecognizer::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaUnregisterCustomRecognizer {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct _1_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "name";
    using type = MaaStringView;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_inst, _1_name, ret>;
};
struct function_MaaUnregisterCustomRecognizer {
  constexpr static auto func = MaaUnregisterCustomRecognizer;
  constexpr static const char* name = "MaaUnregisterCustomRecognizer";
  using type = func_type_MaaUnregisterCustomRecognizer;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaUnregisterCustomRecognizer::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaUnregisterCustomRecognizer::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaClearCustomRecognizer {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaClearCustomRecognizer {
  constexpr static auto func = MaaClearCustomRecognizer;
  constexpr static const char* name = "MaaClearCustomRecognizer";
  using type = func_type_MaaClearCustomRecognizer;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaClearCustomRecognizer::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaClearCustomRecognizer::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaRegisterCustomAction {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct _1_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "name";
    using type = MaaStringView;
  };
  struct _2_action {
    constexpr static size_t index = 2;
    constexpr static const char* name = "action";
    using type = MaaCustomActionHandle;
  };
  struct _3_action_arg {
    constexpr static size_t index = 3;
    constexpr static const char* name = "action_arg";
    using type = MaaTransparentArg;
  };
  struct ret {
    constexpr static size_t index = 4;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_inst, _1_name, _2_action, _3_action_arg, ret>;
};
struct function_MaaRegisterCustomAction {
  constexpr static auto func = MaaRegisterCustomAction;
  constexpr static const char* name = "MaaRegisterCustomAction";
  using type = func_type_MaaRegisterCustomAction;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaRegisterCustomAction::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaRegisterCustomAction::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaUnregisterCustomAction {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct _1_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "name";
    using type = MaaStringView;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_inst, _1_name, ret>;
};
struct function_MaaUnregisterCustomAction {
  constexpr static auto func = MaaUnregisterCustomAction;
  constexpr static const char* name = "MaaUnregisterCustomAction";
  using type = func_type_MaaUnregisterCustomAction;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaUnregisterCustomAction::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaUnregisterCustomAction::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaClearCustomAction {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaClearCustomAction {
  constexpr static auto func = MaaClearCustomAction;
  constexpr static const char* name = "MaaClearCustomAction";
  using type = func_type_MaaClearCustomAction;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaClearCustomAction::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaClearCustomAction::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaPostTask {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct _1_entry {
    constexpr static size_t index = 1;
    constexpr static const char* name = "entry";
    using type = MaaStringView;
  };
  struct _2_param {
    constexpr static size_t index = 2;
    constexpr static const char* name = "param";
    using type = MaaStringView;
  };
  struct ret {
    constexpr static size_t index = 3;
    constexpr static const char* name = "return";
    using type = MaaTaskId;
  };
  using args = std::tuple<_0_inst, _1_entry, _2_param, ret>;
};
struct function_MaaPostTask {
  constexpr static auto func = MaaPostTask;
  constexpr static const char* name = "MaaPostTask";
  using type = func_type_MaaPostTask;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaPostTask::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaPostTask::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaSetTaskParam {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct _1_id {
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = MaaTaskId;
  };
  struct _2_param {
    constexpr static size_t index = 2;
    constexpr static const char* name = "param";
    using type = MaaStringView;
  };
  struct ret {
    constexpr static size_t index = 3;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_inst, _1_id, _2_param, ret>;
};
struct function_MaaSetTaskParam {
  constexpr static auto func = MaaSetTaskParam;
  constexpr static const char* name = "MaaSetTaskParam";
  using type = func_type_MaaSetTaskParam;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaSetTaskParam::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaSetTaskParam::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaTaskStatus {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct _1_id {
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = MaaTaskId;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaStatus;
  };
  using args = std::tuple<_0_inst, _1_id, ret>;
};
struct function_MaaTaskStatus {
  constexpr static auto func = MaaTaskStatus;
  constexpr static const char* name = "MaaTaskStatus";
  using type = func_type_MaaTaskStatus;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaTaskStatus::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaTaskStatus::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaWaitTask {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct _1_id {
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = MaaTaskId;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaStatus;
  };
  using args = std::tuple<_0_inst, _1_id, ret>;
};
struct function_MaaWaitTask {
  constexpr static auto func = MaaWaitTask;
  constexpr static const char* name = "MaaWaitTask";
  using type = func_type_MaaWaitTask;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaWaitTask::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaWaitTask::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaTaskAllFinished {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaTaskAllFinished {
  constexpr static auto func = MaaTaskAllFinished;
  constexpr static const char* name = "MaaTaskAllFinished";
  using type = func_type_MaaTaskAllFinished;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaTaskAllFinished::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaTaskAllFinished::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaPostStop {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaPostStop {
  constexpr static auto func = MaaPostStop;
  constexpr static const char* name = "MaaPostStop";
  using type = func_type_MaaPostStop;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaPostStop::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaPostStop::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaStop {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaStop {
  constexpr static auto func = MaaStop;
  constexpr static const char* name = "MaaStop";
  using type = func_type_MaaStop;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaStop::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaStop::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaGetResource {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaResourceHandle;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaGetResource {
  constexpr static auto func = MaaGetResource;
  constexpr static const char* name = "MaaGetResource";
  using type = func_type_MaaGetResource;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaGetResource::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaGetResource::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaGetController {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaControllerHandle;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaGetController {
  constexpr static auto func = MaaGetController;
  constexpr static const char* name = "MaaGetController";
  using type = func_type_MaaGetController;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaGetController::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaGetController::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaResourceCreate {
  struct _0_callback {
    constexpr static size_t index = 0;
    constexpr static const char* name = "callback";
    using type = MaaResourceCallback;
  };
  struct _1_callback_arg {
    constexpr static size_t index = 1;
    constexpr static const char* name = "callback_arg";
    using type = MaaCallbackTransparentArg;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaResourceHandle;
  };
  using args = std::tuple<_0_callback, _1_callback_arg, ret>;
};
struct function_MaaResourceCreate {
  constexpr static auto func = MaaResourceCreate;
  constexpr static const char* name = "MaaResourceCreate";
  using type = func_type_MaaResourceCreate;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaResourceCreate::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaResourceCreate::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaResourceDestroy {
  struct _0_res {
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = void;
  };
  using args = std::tuple<_0_res, ret>;
};
struct function_MaaResourceDestroy {
  constexpr static auto func = MaaResourceDestroy;
  constexpr static const char* name = "MaaResourceDestroy";
  using type = func_type_MaaResourceDestroy;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaResourceDestroy::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaResourceDestroy::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaResourcePostPath {
  struct _0_res {
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceHandle;
  };
  struct _1_path {
    constexpr static size_t index = 1;
    constexpr static const char* name = "path";
    using type = MaaStringView;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaResId;
  };
  using args = std::tuple<_0_res, _1_path, ret>;
};
struct function_MaaResourcePostPath {
  constexpr static auto func = MaaResourcePostPath;
  constexpr static const char* name = "MaaResourcePostPath";
  using type = func_type_MaaResourcePostPath;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaResourcePostPath::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaResourcePostPath::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaResourceStatus {
  struct _0_res {
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceHandle;
  };
  struct _1_id {
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = MaaResId;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaStatus;
  };
  using args = std::tuple<_0_res, _1_id, ret>;
};
struct function_MaaResourceStatus {
  constexpr static auto func = MaaResourceStatus;
  constexpr static const char* name = "MaaResourceStatus";
  using type = func_type_MaaResourceStatus;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaResourceStatus::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaResourceStatus::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaResourceWait {
  struct _0_res {
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceHandle;
  };
  struct _1_id {
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = MaaResId;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaStatus;
  };
  using args = std::tuple<_0_res, _1_id, ret>;
};
struct function_MaaResourceWait {
  constexpr static auto func = MaaResourceWait;
  constexpr static const char* name = "MaaResourceWait";
  using type = func_type_MaaResourceWait;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaResourceWait::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaResourceWait::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaResourceLoaded {
  struct _0_res {
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_res, ret>;
};
struct function_MaaResourceLoaded {
  constexpr static auto func = MaaResourceLoaded;
  constexpr static const char* name = "MaaResourceLoaded";
  using type = func_type_MaaResourceLoaded;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaResourceLoaded::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaResourceLoaded::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaResourceGetHash {
  struct _0_res {
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceHandle;
  };
  struct _1_buffer {
    constexpr static size_t index = 1;
    constexpr static const char* name = "buffer";
    using type = MaaStringBufferHandle;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_res, _1_buffer, ret>;
};
struct function_MaaResourceGetHash {
  constexpr static auto func = MaaResourceGetHash;
  constexpr static const char* name = "MaaResourceGetHash";
  using type = func_type_MaaResourceGetHash;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaResourceGetHash::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaResourceGetHash::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaResourceGetTaskList {
  struct _0_res {
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceHandle;
  };
  struct _1_buffer {
    constexpr static size_t index = 1;
    constexpr static const char* name = "buffer";
    using type = MaaStringBufferHandle;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_res, _1_buffer, ret>;
};
struct function_MaaResourceGetTaskList {
  constexpr static auto func = MaaResourceGetTaskList;
  constexpr static const char* name = "MaaResourceGetTaskList";
  using type = func_type_MaaResourceGetTaskList;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaResourceGetTaskList::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaResourceGetTaskList::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaCreateImageBuffer {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = MaaImageBufferHandle;
  };
  using args = std::tuple<ret>;
};
struct function_MaaCreateImageBuffer {
  constexpr static auto func = MaaCreateImageBuffer;
  constexpr static const char* name = "MaaCreateImageBuffer";
  using type = func_type_MaaCreateImageBuffer;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaCreateImageBuffer::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaCreateImageBuffer::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaDestroyImageBuffer {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBufferHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = void;
  };
  using args = std::tuple<_0_handle, ret>;
};
struct function_MaaDestroyImageBuffer {
  constexpr static auto func = MaaDestroyImageBuffer;
  constexpr static const char* name = "MaaDestroyImageBuffer";
  using type = func_type_MaaDestroyImageBuffer;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaDestroyImageBuffer::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaDestroyImageBuffer::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaIsImageEmpty {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBufferHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_handle, ret>;
};
struct function_MaaIsImageEmpty {
  constexpr static auto func = MaaIsImageEmpty;
  constexpr static const char* name = "MaaIsImageEmpty";
  using type = func_type_MaaIsImageEmpty;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaIsImageEmpty::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaIsImageEmpty::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaClearImage {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBufferHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_handle, ret>;
};
struct function_MaaClearImage {
  constexpr static auto func = MaaClearImage;
  constexpr static const char* name = "MaaClearImage";
  using type = func_type_MaaClearImage;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaClearImage::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaClearImage::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaGetImageWidth {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBufferHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = int32_t;
  };
  using args = std::tuple<_0_handle, ret>;
};
struct function_MaaGetImageWidth {
  constexpr static auto func = MaaGetImageWidth;
  constexpr static const char* name = "MaaGetImageWidth";
  using type = func_type_MaaGetImageWidth;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaGetImageWidth::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaGetImageWidth::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaGetImageHeight {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBufferHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = int32_t;
  };
  using args = std::tuple<_0_handle, ret>;
};
struct function_MaaGetImageHeight {
  constexpr static auto func = MaaGetImageHeight;
  constexpr static const char* name = "MaaGetImageHeight";
  using type = func_type_MaaGetImageHeight;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaGetImageHeight::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaGetImageHeight::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaGetImageType {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBufferHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = int32_t;
  };
  using args = std::tuple<_0_handle, ret>;
};
struct function_MaaGetImageType {
  constexpr static auto func = MaaGetImageType;
  constexpr static const char* name = "MaaGetImageType";
  using type = func_type_MaaGetImageType;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaGetImageType::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaGetImageType::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaGetImageEncoded {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBufferHandle;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaImageEncodedData;
  };
  using args = std::tuple<_0_handle, ret>;
};
struct function_MaaGetImageEncoded {
  constexpr static auto func = MaaGetImageEncoded;
  constexpr static const char* name = "MaaGetImageEncoded";
  using type = func_type_MaaGetImageEncoded;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaGetImageEncoded::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaGetImageEncoded::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaSetImageEncoded {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBufferHandle;
  };
  struct _1_data {
    constexpr static size_t index = 1;
    constexpr static const char* name = "data";
    using type = MaaImageEncodedData;
  };
  struct _2_size {
    constexpr static size_t index = 2;
    constexpr static const char* name = "size";
    using type = MaaSize;
  };
  struct ret {
    constexpr static size_t index = 3;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_handle, _1_data, _2_size, ret>;
};
struct function_MaaSetImageEncoded {
  constexpr static auto func = MaaSetImageEncoded;
  constexpr static const char* name = "MaaSetImageEncoded";
  using type = func_type_MaaSetImageEncoded;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaSetImageEncoded::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaSetImageEncoded::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaVersion {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = MaaStringView;
  };
  using args = std::tuple<ret>;
};
struct function_MaaVersion {
  constexpr static auto func = MaaVersion;
  constexpr static const char* name = "MaaVersion";
  using type = func_type_MaaVersion;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaVersion::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaVersion::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitInit {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitInit {
  constexpr static auto func = MaaToolkitInit;
  constexpr static const char* name = "MaaToolkitInit";
  using type = func_type_MaaToolkitInit;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitInit::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitInit::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitUninit {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitUninit {
  constexpr static auto func = MaaToolkitUninit;
  constexpr static const char* name = "MaaToolkitUninit";
  using type = func_type_MaaToolkitUninit;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitUninit::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitUninit::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitFindDevice {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = MaaSize;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitFindDevice {
  constexpr static auto func = MaaToolkitFindDevice;
  constexpr static const char* name = "MaaToolkitFindDevice";
  using type = func_type_MaaToolkitFindDevice;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitFindDevice::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitFindDevice::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitFindDeviceWithAdb {
  struct _0_adb_path {
    constexpr static size_t index = 0;
    constexpr static const char* name = "adb_path";
    using type = MaaStringView;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaSize;
  };
  using args = std::tuple<_0_adb_path, ret>;
};
struct function_MaaToolkitFindDeviceWithAdb {
  constexpr static auto func = MaaToolkitFindDeviceWithAdb;
  constexpr static const char* name = "MaaToolkitFindDeviceWithAdb";
  using type = func_type_MaaToolkitFindDeviceWithAdb;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitFindDeviceWithAdb::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitFindDeviceWithAdb::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitPostFindDevice {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitPostFindDevice {
  constexpr static auto func = MaaToolkitPostFindDevice;
  constexpr static const char* name = "MaaToolkitPostFindDevice";
  using type = func_type_MaaToolkitPostFindDevice;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitPostFindDevice::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitPostFindDevice::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitPostFindDeviceWithAdb {
  struct _0_adb_path {
    constexpr static size_t index = 0;
    constexpr static const char* name = "adb_path";
    using type = MaaStringView;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_adb_path, ret>;
};
struct function_MaaToolkitPostFindDeviceWithAdb {
  constexpr static auto func = MaaToolkitPostFindDeviceWithAdb;
  constexpr static const char* name = "MaaToolkitPostFindDeviceWithAdb";
  using type = func_type_MaaToolkitPostFindDeviceWithAdb;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitPostFindDeviceWithAdb::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitPostFindDeviceWithAdb::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitIsFindDeviceCompleted {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitIsFindDeviceCompleted {
  constexpr static auto func = MaaToolkitIsFindDeviceCompleted;
  constexpr static const char* name = "MaaToolkitIsFindDeviceCompleted";
  using type = func_type_MaaToolkitIsFindDeviceCompleted;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitIsFindDeviceCompleted::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitIsFindDeviceCompleted::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitWaitForFindDeviceToComplete {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = MaaSize;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitWaitForFindDeviceToComplete {
  constexpr static auto func = MaaToolkitWaitForFindDeviceToComplete;
  constexpr static const char* name = "MaaToolkitWaitForFindDeviceToComplete";
  using type = func_type_MaaToolkitWaitForFindDeviceToComplete;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitWaitForFindDeviceToComplete::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitWaitForFindDeviceToComplete::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitGetDeviceCount {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = MaaSize;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitGetDeviceCount {
  constexpr static auto func = MaaToolkitGetDeviceCount;
  constexpr static const char* name = "MaaToolkitGetDeviceCount";
  using type = func_type_MaaToolkitGetDeviceCount;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitGetDeviceCount::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetDeviceCount::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitGetDeviceName {
  struct _0_index {
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = MaaSize;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaStringView;
  };
  using args = std::tuple<_0_index, ret>;
};
struct function_MaaToolkitGetDeviceName {
  constexpr static auto func = MaaToolkitGetDeviceName;
  constexpr static const char* name = "MaaToolkitGetDeviceName";
  using type = func_type_MaaToolkitGetDeviceName;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitGetDeviceName::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetDeviceName::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitGetDeviceAdbPath {
  struct _0_index {
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = MaaSize;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaStringView;
  };
  using args = std::tuple<_0_index, ret>;
};
struct function_MaaToolkitGetDeviceAdbPath {
  constexpr static auto func = MaaToolkitGetDeviceAdbPath;
  constexpr static const char* name = "MaaToolkitGetDeviceAdbPath";
  using type = func_type_MaaToolkitGetDeviceAdbPath;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitGetDeviceAdbPath::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetDeviceAdbPath::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitGetDeviceAdbSerial {
  struct _0_index {
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = MaaSize;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaStringView;
  };
  using args = std::tuple<_0_index, ret>;
};
struct function_MaaToolkitGetDeviceAdbSerial {
  constexpr static auto func = MaaToolkitGetDeviceAdbSerial;
  constexpr static const char* name = "MaaToolkitGetDeviceAdbSerial";
  using type = func_type_MaaToolkitGetDeviceAdbSerial;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitGetDeviceAdbSerial::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetDeviceAdbSerial::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitGetDeviceAdbControllerType {
  struct _0_index {
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = MaaSize;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaAdbControllerType;
  };
  using args = std::tuple<_0_index, ret>;
};
struct function_MaaToolkitGetDeviceAdbControllerType {
  constexpr static auto func = MaaToolkitGetDeviceAdbControllerType;
  constexpr static const char* name = "MaaToolkitGetDeviceAdbControllerType";
  using type = func_type_MaaToolkitGetDeviceAdbControllerType;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitGetDeviceAdbControllerType::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetDeviceAdbControllerType::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitGetDeviceAdbConfig {
  struct _0_index {
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = MaaSize;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaStringView;
  };
  using args = std::tuple<_0_index, ret>;
};
struct function_MaaToolkitGetDeviceAdbConfig {
  constexpr static auto func = MaaToolkitGetDeviceAdbConfig;
  constexpr static const char* name = "MaaToolkitGetDeviceAdbConfig";
  using type = func_type_MaaToolkitGetDeviceAdbConfig;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitGetDeviceAdbConfig::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetDeviceAdbConfig::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitRegisterCustomRecognizerExecutor {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaInstanceHandle;
  };
  struct _1_recognizer_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "recognizer_name";
    using type = MaaStringView;
  };
  struct _2_recognizer_exec_path {
    constexpr static size_t index = 2;
    constexpr static const char* name = "recognizer_exec_path";
    using type = MaaStringView;
  };
  struct _3_recognizer_exec_param_json {
    constexpr static size_t index = 3;
    constexpr static const char* name = "recognizer_exec_param_json";
    using type = MaaStringView;
  };
  struct ret {
    constexpr static size_t index = 4;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_handle, _1_recognizer_name, _2_recognizer_exec_path, _3_recognizer_exec_param_json, ret>;
};
struct function_MaaToolkitRegisterCustomRecognizerExecutor {
  constexpr static auto func = MaaToolkitRegisterCustomRecognizerExecutor;
  constexpr static const char* name = "MaaToolkitRegisterCustomRecognizerExecutor";
  using type = func_type_MaaToolkitRegisterCustomRecognizerExecutor;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitRegisterCustomRecognizerExecutor::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitRegisterCustomRecognizerExecutor::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitUnregisterCustomRecognizerExecutor {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaInstanceHandle;
  };
  struct _1_recognizer_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "recognizer_name";
    using type = MaaStringView;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_handle, _1_recognizer_name, ret>;
};
struct function_MaaToolkitUnregisterCustomRecognizerExecutor {
  constexpr static auto func = MaaToolkitUnregisterCustomRecognizerExecutor;
  constexpr static const char* name = "MaaToolkitUnregisterCustomRecognizerExecutor";
  using type = func_type_MaaToolkitUnregisterCustomRecognizerExecutor;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitUnregisterCustomRecognizerExecutor::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitUnregisterCustomRecognizerExecutor::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitRegisterCustomActionExecutor {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaInstanceHandle;
  };
  struct _1_action_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "action_name";
    using type = MaaStringView;
  };
  struct _2_action_exec_path {
    constexpr static size_t index = 2;
    constexpr static const char* name = "action_exec_path";
    using type = MaaStringView;
  };
  struct _3_action_exec_param_json {
    constexpr static size_t index = 3;
    constexpr static const char* name = "action_exec_param_json";
    using type = MaaStringView;
  };
  struct ret {
    constexpr static size_t index = 4;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_handle, _1_action_name, _2_action_exec_path, _3_action_exec_param_json, ret>;
};
struct function_MaaToolkitRegisterCustomActionExecutor {
  constexpr static auto func = MaaToolkitRegisterCustomActionExecutor;
  constexpr static const char* name = "MaaToolkitRegisterCustomActionExecutor";
  using type = func_type_MaaToolkitRegisterCustomActionExecutor;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitRegisterCustomActionExecutor::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitRegisterCustomActionExecutor::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitUnregisterCustomActionExecutor {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaInstanceHandle;
  };
  struct _1_action_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "action_name";
    using type = MaaStringView;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_handle, _1_action_name, ret>;
};
struct function_MaaToolkitUnregisterCustomActionExecutor {
  constexpr static auto func = MaaToolkitUnregisterCustomActionExecutor;
  constexpr static const char* name = "MaaToolkitUnregisterCustomActionExecutor";
  using type = func_type_MaaToolkitUnregisterCustomActionExecutor;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitUnregisterCustomActionExecutor::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitUnregisterCustomActionExecutor::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitFindWindow {
  struct _0_class_name {
    constexpr static size_t index = 0;
    constexpr static const char* name = "class_name";
    using type = MaaStringView;
  };
  struct _1_window_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "window_name";
    using type = MaaStringView;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaSize;
  };
  using args = std::tuple<_0_class_name, _1_window_name, ret>;
};
struct function_MaaToolkitFindWindow {
  constexpr static auto func = MaaToolkitFindWindow;
  constexpr static const char* name = "MaaToolkitFindWindow";
  using type = func_type_MaaToolkitFindWindow;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitFindWindow::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitFindWindow::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitSearchWindow {
  struct _0_class_name {
    constexpr static size_t index = 0;
    constexpr static const char* name = "class_name";
    using type = MaaStringView;
  };
  struct _1_window_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "window_name";
    using type = MaaStringView;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaSize;
  };
  using args = std::tuple<_0_class_name, _1_window_name, ret>;
};
struct function_MaaToolkitSearchWindow {
  constexpr static auto func = MaaToolkitSearchWindow;
  constexpr static const char* name = "MaaToolkitSearchWindow";
  using type = func_type_MaaToolkitSearchWindow;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitSearchWindow::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitSearchWindow::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitGetWindow {
  struct _0_index {
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = MaaSize;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaWin32Hwnd;
  };
  using args = std::tuple<_0_index, ret>;
};
struct function_MaaToolkitGetWindow {
  constexpr static auto func = MaaToolkitGetWindow;
  constexpr static const char* name = "MaaToolkitGetWindow";
  using type = func_type_MaaToolkitGetWindow;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitGetWindow::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetWindow::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitGetCursorWindow {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = MaaWin32Hwnd;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitGetCursorWindow {
  constexpr static auto func = MaaToolkitGetCursorWindow;
  constexpr static const char* name = "MaaToolkitGetCursorWindow";
  using type = func_type_MaaToolkitGetCursorWindow;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitGetCursorWindow::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetCursorWindow::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitGetDesktopWindow {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = MaaWin32Hwnd;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitGetDesktopWindow {
  constexpr static auto func = MaaToolkitGetDesktopWindow;
  constexpr static const char* name = "MaaToolkitGetDesktopWindow";
  using type = func_type_MaaToolkitGetDesktopWindow;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitGetDesktopWindow::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetDesktopWindow::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaToolkitGetForegroundWindow {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = MaaWin32Hwnd;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitGetForegroundWindow {
  constexpr static auto func = MaaToolkitGetForegroundWindow;
  constexpr static const char* name = "MaaToolkitGetForegroundWindow";
  using type = func_type_MaaToolkitGetForegroundWindow;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaToolkitGetForegroundWindow::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetForegroundWindow::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerSetOptionString {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct _1_key {
    constexpr static size_t index = 1;
    constexpr static const char* name = "key";
    using type = MaaCtrlOption;
  };
  struct _2_value {
    constexpr static size_t index = 2;
    constexpr static const char* name = "value";
    using type = MaaStringView;
  };
  struct ret {
    constexpr static size_t index = 3;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_ctrl, _1_key, _2_value, ret>;
};
struct function_MaaControllerSetOptionString {
  constexpr static auto func = MaaControllerSetOptionString;
  constexpr static const char* name = "MaaControllerSetOptionString";
  using type = func_type_MaaControllerSetOptionString;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerSetOptionString::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerSetOptionString::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerSetOptionInteger {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct _1_key {
    constexpr static size_t index = 1;
    constexpr static const char* name = "key";
    using type = MaaCtrlOption;
  };
  struct _2_value {
    constexpr static size_t index = 2;
    constexpr static const char* name = "value";
    using type = int;
  };
  struct ret {
    constexpr static size_t index = 3;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_ctrl, _1_key, _2_value, ret>;
};
struct function_MaaControllerSetOptionInteger {
  constexpr static auto func = MaaControllerSetOptionInteger;
  constexpr static const char* name = "MaaControllerSetOptionInteger";
  using type = func_type_MaaControllerSetOptionInteger;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerSetOptionInteger::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerSetOptionInteger::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaControllerSetOptionBoolean {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerHandle;
  };
  struct _1_key {
    constexpr static size_t index = 1;
    constexpr static const char* name = "key";
    using type = MaaCtrlOption;
  };
  struct _2_value {
    constexpr static size_t index = 2;
    constexpr static const char* name = "value";
    using type = bool;
  };
  struct ret {
    constexpr static size_t index = 3;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_ctrl, _1_key, _2_value, ret>;
};
struct function_MaaControllerSetOptionBoolean {
  constexpr static auto func = MaaControllerSetOptionBoolean;
  constexpr static const char* name = "MaaControllerSetOptionBoolean";
  using type = func_type_MaaControllerSetOptionBoolean;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaControllerSetOptionBoolean::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaControllerSetOptionBoolean::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaSetGlobalOptionString {
  struct _0_key {
    constexpr static size_t index = 0;
    constexpr static const char* name = "key";
    using type = MaaCtrlOption;
  };
  struct _1_value {
    constexpr static size_t index = 1;
    constexpr static const char* name = "value";
    using type = MaaStringView;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_key, _1_value, ret>;
};
struct function_MaaSetGlobalOptionString {
  constexpr static auto func = MaaSetGlobalOptionString;
  constexpr static const char* name = "MaaSetGlobalOptionString";
  using type = func_type_MaaSetGlobalOptionString;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaSetGlobalOptionString::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaSetGlobalOptionString::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaSetGlobalOptionInteger {
  struct _0_key {
    constexpr static size_t index = 0;
    constexpr static const char* name = "key";
    using type = MaaCtrlOption;
  };
  struct _1_value {
    constexpr static size_t index = 1;
    constexpr static const char* name = "value";
    using type = int;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_key, _1_value, ret>;
};
struct function_MaaSetGlobalOptionInteger {
  constexpr static auto func = MaaSetGlobalOptionInteger;
  constexpr static const char* name = "MaaSetGlobalOptionInteger";
  using type = func_type_MaaSetGlobalOptionInteger;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaSetGlobalOptionInteger::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaSetGlobalOptionInteger::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_MaaSetGlobalOptionBoolean {
  struct _0_key {
    constexpr static size_t index = 0;
    constexpr static const char* name = "key";
    using type = MaaCtrlOption;
  };
  struct _1_value {
    constexpr static size_t index = 1;
    constexpr static const char* name = "value";
    using type = bool;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaBool;
  };
  using args = std::tuple<_0_key, _1_value, ret>;
};
struct function_MaaSetGlobalOptionBoolean {
  constexpr static auto func = MaaSetGlobalOptionBoolean;
  constexpr static const char* name = "MaaSetGlobalOptionBoolean";
  using type = func_type_MaaSetGlobalOptionBoolean;
};

}
namespace lhg::call {

template<>
struct is_input<maa::func_type_MaaSetGlobalOptionBoolean::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaSetGlobalOptionBoolean::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

using __function_list = std::tuple<
  function_MaaAdbControllerCreate,
  function_MaaWin32ControllerCreate,
  function_MaaAdbControllerCreateV2,
  function_MaaThriftControllerCreate,
  function_MaaDbgControllerCreate,
  function_MaaControllerDestroy,
  function_MaaControllerPostConnection,
  function_MaaControllerPostClick,
  function_MaaControllerPostSwipe,
  function_MaaControllerPostPressKey,
  function_MaaControllerPostInputText,
  function_MaaControllerPostTouchDown,
  function_MaaControllerPostTouchMove,
  function_MaaControllerPostTouchUp,
  function_MaaControllerPostScreencap,
  function_MaaControllerStatus,
  function_MaaControllerWait,
  function_MaaControllerConnected,
  function_MaaControllerGetImage,
  function_MaaControllerGetUUID,
  function_MaaCreate,
  function_MaaDestroy,
  function_MaaBindResource,
  function_MaaBindController,
  function_MaaInited,
  function_MaaRegisterCustomRecognizer,
  function_MaaUnregisterCustomRecognizer,
  function_MaaClearCustomRecognizer,
  function_MaaRegisterCustomAction,
  function_MaaUnregisterCustomAction,
  function_MaaClearCustomAction,
  function_MaaPostTask,
  function_MaaSetTaskParam,
  function_MaaTaskStatus,
  function_MaaWaitTask,
  function_MaaTaskAllFinished,
  function_MaaPostStop,
  function_MaaStop,
  function_MaaGetResource,
  function_MaaGetController,
  function_MaaResourceCreate,
  function_MaaResourceDestroy,
  function_MaaResourcePostPath,
  function_MaaResourceStatus,
  function_MaaResourceWait,
  function_MaaResourceLoaded,
  function_MaaResourceGetHash,
  function_MaaResourceGetTaskList,
  function_MaaCreateImageBuffer,
  function_MaaDestroyImageBuffer,
  function_MaaIsImageEmpty,
  function_MaaClearImage,
  function_MaaGetImageWidth,
  function_MaaGetImageHeight,
  function_MaaGetImageType,
  function_MaaGetImageEncoded,
  function_MaaSetImageEncoded,
  function_MaaVersion,
  function_MaaToolkitInit,
  function_MaaToolkitUninit,
  function_MaaToolkitFindDevice,
  function_MaaToolkitFindDeviceWithAdb,
  function_MaaToolkitPostFindDevice,
  function_MaaToolkitPostFindDeviceWithAdb,
  function_MaaToolkitIsFindDeviceCompleted,
  function_MaaToolkitWaitForFindDeviceToComplete,
  function_MaaToolkitGetDeviceCount,
  function_MaaToolkitGetDeviceName,
  function_MaaToolkitGetDeviceAdbPath,
  function_MaaToolkitGetDeviceAdbSerial,
  function_MaaToolkitGetDeviceAdbControllerType,
  function_MaaToolkitGetDeviceAdbConfig,
  function_MaaToolkitRegisterCustomRecognizerExecutor,
  function_MaaToolkitUnregisterCustomRecognizerExecutor,
  function_MaaToolkitRegisterCustomActionExecutor,
  function_MaaToolkitUnregisterCustomActionExecutor,
  function_MaaToolkitFindWindow,
  function_MaaToolkitSearchWindow,
  function_MaaToolkitGetWindow,
  function_MaaToolkitGetCursorWindow,
  function_MaaToolkitGetDesktopWindow,
  function_MaaToolkitGetForegroundWindow,
  function_MaaControllerSetOptionString,
  function_MaaControllerSetOptionInteger,
  function_MaaControllerSetOptionBoolean,
  function_MaaSetGlobalOptionString,
  function_MaaSetGlobalOptionInteger,
  function_MaaSetGlobalOptionBoolean
>;

}
namespace lhg::maa {

struct func_type_MaaAPICallback {
  struct _0_msg {
    constexpr static size_t index = 0;
    constexpr static const char* name = "msg";
    using type = const char *;
  };
  struct _1_details_json {
    constexpr static size_t index = 1;
    constexpr static const char* name = "details_json";
    using type =  const char *;
  };
  struct _2_ {
    constexpr static size_t index = 2;
    constexpr static const char* name = "";
    using type =  void *;
  };
  struct ret {
    constexpr static size_t index = 3;
    constexpr static const char* name = "return";
    using type = void;
  };
  using args = std::tuple<_0_msg, _1_details_json, _2_, ret>;
};
struct callback_MaaAPICallback {
  using type = func_type_MaaAPICallback;
  using func_type = void (*)(const char *, const char *, void *);
  constexpr static size_t context = 2;
};

}
namespace lhg::callback {

template<>
struct is_input<maa::func_type_MaaAPICallback::_2_, false> {
  constexpr static bool value = false;
};
template<>
struct is_input<maa::func_type_MaaAPICallback::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_MaaAPICallback::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_CustomActionRun {
  struct _0_sync_context {
    constexpr static size_t index = 0;
    constexpr static const char* name = "sync_context";
    using type = MaaSyncContextAPI *;
  };
  struct _1_task_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "task_name";
    using type =  const char *;
  };
  struct _2_custom_action_param {
    constexpr static size_t index = 2;
    constexpr static const char* name = "custom_action_param";
    using type =  const char *;
  };
  struct _3_cur_box {
    constexpr static size_t index = 3;
    constexpr static const char* name = "cur_box";
    using type =  MaaRect *;
  };
  struct _4_cur_rec_detail {
    constexpr static size_t index = 4;
    constexpr static const char* name = "cur_rec_detail";
    using type =  const char *;
  };
  struct _5_ {
    constexpr static size_t index = 5;
    constexpr static const char* name = "";
    using type =  void *;
  };
  struct ret {
    constexpr static size_t index = 6;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_sync_context, _1_task_name, _2_custom_action_param, _3_cur_box, _4_cur_rec_detail, _5_, ret>;
};
struct callback_CustomActionRun {
  using type = func_type_CustomActionRun;
  using func_type = unsigned char (*)(MaaSyncContextAPI *, const char *, const char *, MaaRect *, const char *, void *);
  constexpr static size_t context = 5;
};

}
namespace lhg::callback {

template<>
struct is_input<maa::func_type_CustomActionRun::_5_, false> {
  constexpr static bool value = false;
};
template<>
struct is_input<maa::func_type_CustomActionRun::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_CustomActionRun::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_CustomActionStop {
  struct _0_ {
    constexpr static size_t index = 0;
    constexpr static const char* name = "";
    using type = void *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = void;
  };
  using args = std::tuple<_0_, ret>;
};
struct callback_CustomActionStop {
  using type = func_type_CustomActionStop;
  using func_type = void (*)(void *);
  constexpr static size_t context = 0;
};

}
namespace lhg::callback {

template<>
struct is_input<maa::func_type_CustomActionStop::_0_, false> {
  constexpr static bool value = false;
};
template<>
struct is_input<maa::func_type_CustomActionStop::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_CustomActionStop::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::maa {

struct func_type_CustomRecognizerAnalyze {
  struct _0_sync_context {
    constexpr static size_t index = 0;
    constexpr static const char* name = "sync_context";
    using type = MaaSyncContextAPI *;
  };
  struct _1_image {
    constexpr static size_t index = 1;
    constexpr static const char* name = "image";
    using type =  MaaImageBuffer *;
  };
  struct _2_task_name {
    constexpr static size_t index = 2;
    constexpr static const char* name = "task_name";
    using type =  const char *;
  };
  struct _3_custom_recognition_param {
    constexpr static size_t index = 3;
    constexpr static const char* name = "custom_recognition_param";
    using type =  const char *;
  };
  struct _4_ {
    constexpr static size_t index = 4;
    constexpr static const char* name = "";
    using type =  void *;
  };
  struct _5_out_box {
    constexpr static size_t index = 5;
    constexpr static const char* name = "out_box";
    using type =  MaaRect *;
  };
  struct _6_out_detail {
    constexpr static size_t index = 6;
    constexpr static const char* name = "out_detail";
    using type =  MaaStringBuffer *;
  };
  struct ret {
    constexpr static size_t index = 7;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_sync_context, _1_image, _2_task_name, _3_custom_recognition_param, _4_, _5_out_box, _6_out_detail, ret>;
};
struct callback_CustomRecognizerAnalyze {
  using type = func_type_CustomRecognizerAnalyze;
  using func_type = unsigned char (*)(MaaSyncContextAPI *, MaaImageBuffer *, const char *, const char *, void *, MaaRect *, MaaStringBuffer *);
  constexpr static size_t context = 4;
};

}
namespace lhg::callback {

template<>
struct is_input<maa::func_type_CustomRecognizerAnalyze::_4_, false> {
  constexpr static bool value = false;
};
template<>
struct is_input<maa::func_type_CustomRecognizerAnalyze::ret, false> {
  constexpr static bool value = false;
};
template<>
struct is_output<maa::func_type_CustomRecognizerAnalyze::ret, false> {
  constexpr static bool value = true;
};

}

namespace lhg::call {

template<>
struct type_is_handle<MaaControllerAPI *, false> {
  constexpr static bool value = true;
};

}
namespace lhg::callback {

template<>
struct type_is_handle<MaaControllerAPI *, false> {
  constexpr static bool value = true;
};

}

namespace lhg::call {

template<>
struct type_is_handle<MaaResourceAPI *, false> {
  constexpr static bool value = true;
};

}
namespace lhg::callback {

template<>
struct type_is_handle<MaaResourceAPI *, false> {
  constexpr static bool value = true;
};

}

namespace lhg::call {

template<>
struct type_is_handle<MaaInstanceAPI *, false> {
  constexpr static bool value = true;
};

}
namespace lhg::callback {

template<>
struct type_is_handle<MaaInstanceAPI *, false> {
  constexpr static bool value = true;
};

}

namespace lhg::call {

template<>
struct type_is_handle<MaaImageBuffer *, false> {
  constexpr static bool value = true;
};

}
namespace lhg::callback {

template<>
struct type_is_handle<MaaImageBuffer *, false> {
  constexpr static bool value = true;
};

}

namespace lhg::call {

template<>
struct type_is_handle<MaaSyncContextAPI *, false> {
  constexpr static bool value = true;
};

}
namespace lhg::callback {

template<>
struct type_is_handle<MaaSyncContextAPI *, false> {
  constexpr static bool value = true;
};

}
