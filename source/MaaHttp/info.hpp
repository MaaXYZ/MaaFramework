#pragma once

#include "function/interface.hpp"

#include "include.h"

namespace lhg {

namespace maa {

struct func_type_MaaAdbControllerCreate {
  struct _0_adb_path {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "adb_path";
    using type = const char *;
  };
  struct _1_address {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "address";
    using type = const char *;
  };
  struct _2_type {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "type";
    using type = int;
  };
  struct _3_config {
    constexpr static bool ret = false;
    constexpr static size_t index = 3;
    constexpr static const char* name = "config";
    using type = const char *;
  };
  struct _4_callback {
    constexpr static bool ret = false;
    constexpr static size_t index = 4;
    constexpr static const char* name = "callback";
    using type = void (*)(const char *, const char *, void *);
  };
  struct _5_callback_arg {
    constexpr static bool ret = false;
    constexpr static size_t index = 5;
    constexpr static const char* name = "callback_arg";
    using type = void *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 6;
    using type = MaaControllerAPI *;
  };
  using args = std::tuple<_0_adb_path, _1_address, _2_type, _3_config, _4_callback, _5_callback_arg, _ret>;
};
struct function_MaaAdbControllerCreate {
  constexpr static auto func = MaaAdbControllerCreate;
  using type = func_type_MaaAdbControllerCreate;
};

struct func_type_MaaAdbControllerCreateV2 {
  struct _0_adb_path {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "adb_path";
    using type = const char *;
  };
  struct _1_address {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "address";
    using type = const char *;
  };
  struct _2_type {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "type";
    using type = int;
  };
  struct _3_config {
    constexpr static bool ret = false;
    constexpr static size_t index = 3;
    constexpr static const char* name = "config";
    using type = const char *;
  };
  struct _4_agent_path {
    constexpr static bool ret = false;
    constexpr static size_t index = 4;
    constexpr static const char* name = "agent_path";
    using type = const char *;
  };
  struct _5_callback {
    constexpr static bool ret = false;
    constexpr static size_t index = 5;
    constexpr static const char* name = "callback";
    using type = void (*)(const char *, const char *, void *);
  };
  struct _6_callback_arg {
    constexpr static bool ret = false;
    constexpr static size_t index = 6;
    constexpr static const char* name = "callback_arg";
    using type = void *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 7;
    using type = MaaControllerAPI *;
  };
  using args = std::tuple<_0_adb_path, _1_address, _2_type, _3_config, _4_agent_path, _5_callback, _6_callback_arg, _ret>;
};
struct function_MaaAdbControllerCreateV2 {
  constexpr static auto func = MaaAdbControllerCreateV2;
  using type = func_type_MaaAdbControllerCreateV2;
};

struct func_type_MaaThriftControllerCreate {
  struct _0_type {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "type";
    using type = int;
  };
  struct _1_host {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "host";
    using type = const char *;
  };
  struct _2_port {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "port";
    using type = int;
  };
  struct _3_config {
    constexpr static bool ret = false;
    constexpr static size_t index = 3;
    constexpr static const char* name = "config";
    using type = const char *;
  };
  struct _4_callback {
    constexpr static bool ret = false;
    constexpr static size_t index = 4;
    constexpr static const char* name = "callback";
    using type = void (*)(const char *, const char *, void *);
  };
  struct _5_callback_arg {
    constexpr static bool ret = false;
    constexpr static size_t index = 5;
    constexpr static const char* name = "callback_arg";
    using type = void *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 6;
    using type = MaaControllerAPI *;
  };
  using args = std::tuple<_0_type, _1_host, _2_port, _3_config, _4_callback, _5_callback_arg, _ret>;
};
struct function_MaaThriftControllerCreate {
  constexpr static auto func = MaaThriftControllerCreate;
  using type = func_type_MaaThriftControllerCreate;
};

struct func_type_MaaDbgControllerCreate {
  struct _0_read_path {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "read_path";
    using type = const char *;
  };
  struct _1_write_path {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "write_path";
    using type = const char *;
  };
  struct _2_type {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "type";
    using type = int;
  };
  struct _3_config {
    constexpr static bool ret = false;
    constexpr static size_t index = 3;
    constexpr static const char* name = "config";
    using type = const char *;
  };
  struct _4_callback {
    constexpr static bool ret = false;
    constexpr static size_t index = 4;
    constexpr static const char* name = "callback";
    using type = void (*)(const char *, const char *, void *);
  };
  struct _5_callback_arg {
    constexpr static bool ret = false;
    constexpr static size_t index = 5;
    constexpr static const char* name = "callback_arg";
    using type = void *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 6;
    using type = MaaControllerAPI *;
  };
  using args = std::tuple<_0_read_path, _1_write_path, _2_type, _3_config, _4_callback, _5_callback_arg, _ret>;
};
struct function_MaaDbgControllerCreate {
  constexpr static auto func = MaaDbgControllerCreate;
  using type = func_type_MaaDbgControllerCreate;
};

struct func_type_MaaControllerDestroy {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = void;
  };
  using args = std::tuple<_0_ctrl, _ret>;
};
struct function_MaaControllerDestroy {
  constexpr static auto func = MaaControllerDestroy;
  using type = func_type_MaaControllerDestroy;
};

struct func_type_MaaControllerPostConnection {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, _ret>;
};
struct function_MaaControllerPostConnection {
  constexpr static auto func = MaaControllerPostConnection;
  using type = func_type_MaaControllerPostConnection;
};

struct func_type_MaaControllerPostClick {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_x {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "x";
    using type = int;
  };
  struct _2_y {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "y";
    using type = int;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 3;
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, _1_x, _2_y, _ret>;
};
struct function_MaaControllerPostClick {
  constexpr static auto func = MaaControllerPostClick;
  using type = func_type_MaaControllerPostClick;
};

struct func_type_MaaControllerPostSwipe {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_x1 {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "x1";
    using type = int;
  };
  struct _2_y1 {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "y1";
    using type = int;
  };
  struct _3_x2 {
    constexpr static bool ret = false;
    constexpr static size_t index = 3;
    constexpr static const char* name = "x2";
    using type = int;
  };
  struct _4_y2 {
    constexpr static bool ret = false;
    constexpr static size_t index = 4;
    constexpr static const char* name = "y2";
    using type = int;
  };
  struct _5_duration {
    constexpr static bool ret = false;
    constexpr static size_t index = 5;
    constexpr static const char* name = "duration";
    using type = int;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 6;
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, _1_x1, _2_y1, _3_x2, _4_y2, _5_duration, _ret>;
};
struct function_MaaControllerPostSwipe {
  constexpr static auto func = MaaControllerPostSwipe;
  using type = func_type_MaaControllerPostSwipe;
};

struct func_type_MaaControllerPostPressKey {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_keycode {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "keycode";
    using type = int;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, _1_keycode, _ret>;
};
struct function_MaaControllerPostPressKey {
  constexpr static auto func = MaaControllerPostPressKey;
  using type = func_type_MaaControllerPostPressKey;
};

struct func_type_MaaControllerPostInputText {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_text {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "text";
    using type = const char *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, _1_text, _ret>;
};
struct function_MaaControllerPostInputText {
  constexpr static auto func = MaaControllerPostInputText;
  using type = func_type_MaaControllerPostInputText;
};

struct func_type_MaaControllerPostTouchDown {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_contact {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "contact";
    using type = int;
  };
  struct _2_x {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "x";
    using type = int;
  };
  struct _3_y {
    constexpr static bool ret = false;
    constexpr static size_t index = 3;
    constexpr static const char* name = "y";
    using type = int;
  };
  struct _4_pressure {
    constexpr static bool ret = false;
    constexpr static size_t index = 4;
    constexpr static const char* name = "pressure";
    using type = int;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 5;
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, _1_contact, _2_x, _3_y, _4_pressure, _ret>;
};
struct function_MaaControllerPostTouchDown {
  constexpr static auto func = MaaControllerPostTouchDown;
  using type = func_type_MaaControllerPostTouchDown;
};

struct func_type_MaaControllerPostTouchMove {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_contact {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "contact";
    using type = int;
  };
  struct _2_x {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "x";
    using type = int;
  };
  struct _3_y {
    constexpr static bool ret = false;
    constexpr static size_t index = 3;
    constexpr static const char* name = "y";
    using type = int;
  };
  struct _4_pressure {
    constexpr static bool ret = false;
    constexpr static size_t index = 4;
    constexpr static const char* name = "pressure";
    using type = int;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 5;
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, _1_contact, _2_x, _3_y, _4_pressure, _ret>;
};
struct function_MaaControllerPostTouchMove {
  constexpr static auto func = MaaControllerPostTouchMove;
  using type = func_type_MaaControllerPostTouchMove;
};

struct func_type_MaaControllerPostTouchUp {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_contact {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "contact";
    using type = int;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, _1_contact, _ret>;
};
struct function_MaaControllerPostTouchUp {
  constexpr static auto func = MaaControllerPostTouchUp;
  using type = func_type_MaaControllerPostTouchUp;
};

struct func_type_MaaControllerPostScreencap {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, _ret>;
};
struct function_MaaControllerPostScreencap {
  constexpr static auto func = MaaControllerPostScreencap;
  using type = func_type_MaaControllerPostScreencap;
};

struct func_type_MaaControllerStatus {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_id {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = long long;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = int;
  };
  using args = std::tuple<_0_ctrl, _1_id, _ret>;
};
struct function_MaaControllerStatus {
  constexpr static auto func = MaaControllerStatus;
  using type = func_type_MaaControllerStatus;
};

struct func_type_MaaControllerWait {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_id {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = long long;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = int;
  };
  using args = std::tuple<_0_ctrl, _1_id, _ret>;
};
struct function_MaaControllerWait {
  constexpr static auto func = MaaControllerWait;
  using type = func_type_MaaControllerWait;
};

struct func_type_MaaControllerConnected {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = unsigned char;
  };
  using args = std::tuple<_0_ctrl, _ret>;
};
struct function_MaaControllerConnected {
  constexpr static auto func = MaaControllerConnected;
  using type = func_type_MaaControllerConnected;
};

struct func_type_MaaControllerGetImage {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_buffer {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "buffer";
    using type = MaaImageBuffer *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = unsigned char;
  };
  using args = std::tuple<_0_ctrl, _1_buffer, _ret>;
};
struct function_MaaControllerGetImage {
  constexpr static auto func = MaaControllerGetImage;
  using type = func_type_MaaControllerGetImage;
};

struct func_type_MaaControllerGetUUID {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_buffer {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "buffer";
    using type = MaaStringBuffer *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = unsigned char;
  };
  using args = std::tuple<_0_ctrl, _1_buffer, _ret>;
};
struct function_MaaControllerGetUUID {
  constexpr static auto func = MaaControllerGetUUID;
  using type = func_type_MaaControllerGetUUID;
};

struct func_type_MaaCreate {
  struct _0_callback {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "callback";
    using type = void (*)(const char *, const char *, void *);
  };
  struct _1_callback_arg {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "callback_arg";
    using type = void *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = MaaInstanceAPI *;
  };
  using args = std::tuple<_0_callback, _1_callback_arg, _ret>;
};
struct function_MaaCreate {
  constexpr static auto func = MaaCreate;
  using type = func_type_MaaCreate;
};

struct func_type_MaaDestroy {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = void;
  };
  using args = std::tuple<_0_inst, _ret>;
};
struct function_MaaDestroy {
  constexpr static auto func = MaaDestroy;
  using type = func_type_MaaDestroy;
};

struct func_type_MaaBindResource {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_res {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "res";
    using type = MaaResourceAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _1_res, _ret>;
};
struct function_MaaBindResource {
  constexpr static auto func = MaaBindResource;
  using type = func_type_MaaBindResource;
};

struct func_type_MaaBindController {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _1_ctrl, _ret>;
};
struct function_MaaBindController {
  constexpr static auto func = MaaBindController;
  using type = func_type_MaaBindController;
};

struct func_type_MaaInited {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _ret>;
};
struct function_MaaInited {
  constexpr static auto func = MaaInited;
  using type = func_type_MaaInited;
};

struct func_type_MaaUnregisterCustomRecognizer {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_name {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "name";
    using type = const char *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _1_name, _ret>;
};
struct function_MaaUnregisterCustomRecognizer {
  constexpr static auto func = MaaUnregisterCustomRecognizer;
  using type = func_type_MaaUnregisterCustomRecognizer;
};

struct func_type_MaaClearCustomRecognizer {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _ret>;
};
struct function_MaaClearCustomRecognizer {
  constexpr static auto func = MaaClearCustomRecognizer;
  using type = func_type_MaaClearCustomRecognizer;
};

struct func_type_MaaUnregisterCustomAction {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_name {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "name";
    using type = const char *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _1_name, _ret>;
};
struct function_MaaUnregisterCustomAction {
  constexpr static auto func = MaaUnregisterCustomAction;
  using type = func_type_MaaUnregisterCustomAction;
};

struct func_type_MaaClearCustomAction {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _ret>;
};
struct function_MaaClearCustomAction {
  constexpr static auto func = MaaClearCustomAction;
  using type = func_type_MaaClearCustomAction;
};

struct func_type_MaaPostTask {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_entry {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "entry";
    using type = const char *;
  };
  struct _2_param {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "param";
    using type = const char *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 3;
    using type = long long;
  };
  using args = std::tuple<_0_inst, _1_entry, _2_param, _ret>;
};
struct function_MaaPostTask {
  constexpr static auto func = MaaPostTask;
  using type = func_type_MaaPostTask;
};

struct func_type_MaaSetTaskParam {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_id {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = long long;
  };
  struct _2_param {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "param";
    using type = const char *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 3;
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _1_id, _2_param, _ret>;
};
struct function_MaaSetTaskParam {
  constexpr static auto func = MaaSetTaskParam;
  using type = func_type_MaaSetTaskParam;
};

struct func_type_MaaTaskStatus {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_id {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = long long;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = int;
  };
  using args = std::tuple<_0_inst, _1_id, _ret>;
};
struct function_MaaTaskStatus {
  constexpr static auto func = MaaTaskStatus;
  using type = func_type_MaaTaskStatus;
};

struct func_type_MaaWaitTask {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_id {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = long long;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = int;
  };
  using args = std::tuple<_0_inst, _1_id, _ret>;
};
struct function_MaaWaitTask {
  constexpr static auto func = MaaWaitTask;
  using type = func_type_MaaWaitTask;
};

struct func_type_MaaTaskAllFinished {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _ret>;
};
struct function_MaaTaskAllFinished {
  constexpr static auto func = MaaTaskAllFinished;
  using type = func_type_MaaTaskAllFinished;
};

struct func_type_MaaPostStop {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _ret>;
};
struct function_MaaPostStop {
  constexpr static auto func = MaaPostStop;
  using type = func_type_MaaPostStop;
};

struct func_type_MaaStop {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _ret>;
};
struct function_MaaStop {
  constexpr static auto func = MaaStop;
  using type = func_type_MaaStop;
};

struct func_type_MaaGetResource {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = MaaResourceAPI *;
  };
  using args = std::tuple<_0_inst, _ret>;
};
struct function_MaaGetResource {
  constexpr static auto func = MaaGetResource;
  using type = func_type_MaaGetResource;
};

struct func_type_MaaGetController {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = MaaControllerAPI *;
  };
  using args = std::tuple<_0_inst, _ret>;
};
struct function_MaaGetController {
  constexpr static auto func = MaaGetController;
  using type = func_type_MaaGetController;
};

struct func_type_MaaResourceCreate {
  struct _0_callback {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "callback";
    using type = void (*)(const char *, const char *, void *);
  };
  struct _1_callback_arg {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "callback_arg";
    using type = void *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = MaaResourceAPI *;
  };
  using args = std::tuple<_0_callback, _1_callback_arg, _ret>;
};
struct function_MaaResourceCreate {
  constexpr static auto func = MaaResourceCreate;
  using type = func_type_MaaResourceCreate;
};

struct func_type_MaaResourceDestroy {
  struct _0_res {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = void;
  };
  using args = std::tuple<_0_res, _ret>;
};
struct function_MaaResourceDestroy {
  constexpr static auto func = MaaResourceDestroy;
  using type = func_type_MaaResourceDestroy;
};

struct func_type_MaaResourcePostPath {
  struct _0_res {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceAPI *;
  };
  struct _1_path {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "path";
    using type = const char *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = long long;
  };
  using args = std::tuple<_0_res, _1_path, _ret>;
};
struct function_MaaResourcePostPath {
  constexpr static auto func = MaaResourcePostPath;
  using type = func_type_MaaResourcePostPath;
};

struct func_type_MaaResourceStatus {
  struct _0_res {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceAPI *;
  };
  struct _1_id {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = long long;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = int;
  };
  using args = std::tuple<_0_res, _1_id, _ret>;
};
struct function_MaaResourceStatus {
  constexpr static auto func = MaaResourceStatus;
  using type = func_type_MaaResourceStatus;
};

struct func_type_MaaResourceWait {
  struct _0_res {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceAPI *;
  };
  struct _1_id {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = long long;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = int;
  };
  using args = std::tuple<_0_res, _1_id, _ret>;
};
struct function_MaaResourceWait {
  constexpr static auto func = MaaResourceWait;
  using type = func_type_MaaResourceWait;
};

struct func_type_MaaResourceLoaded {
  struct _0_res {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceAPI *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = unsigned char;
  };
  using args = std::tuple<_0_res, _ret>;
};
struct function_MaaResourceLoaded {
  constexpr static auto func = MaaResourceLoaded;
  using type = func_type_MaaResourceLoaded;
};

struct func_type_MaaResourceGetHash {
  struct _0_res {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceAPI *;
  };
  struct _1_buffer {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "buffer";
    using type = MaaStringBuffer *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = unsigned char;
  };
  using args = std::tuple<_0_res, _1_buffer, _ret>;
};
struct function_MaaResourceGetHash {
  constexpr static auto func = MaaResourceGetHash;
  using type = func_type_MaaResourceGetHash;
};

struct func_type_MaaResourceGetTaskList {
  struct _0_res {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceAPI *;
  };
  struct _1_buffer {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "buffer";
    using type = MaaStringBuffer *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = unsigned char;
  };
  using args = std::tuple<_0_res, _1_buffer, _ret>;
};
struct function_MaaResourceGetTaskList {
  constexpr static auto func = MaaResourceGetTaskList;
  using type = func_type_MaaResourceGetTaskList;
};

struct func_type_MaaCreateImageBuffer {
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 0;
    using type = MaaImageBuffer *;
  };
  using args = std::tuple<_ret>;
};
struct function_MaaCreateImageBuffer {
  constexpr static auto func = MaaCreateImageBuffer;
  using type = func_type_MaaCreateImageBuffer;
};

struct func_type_MaaDestroyImageBuffer {
  struct _0_handle {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBuffer *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = void;
  };
  using args = std::tuple<_0_handle, _ret>;
};
struct function_MaaDestroyImageBuffer {
  constexpr static auto func = MaaDestroyImageBuffer;
  using type = func_type_MaaDestroyImageBuffer;
};

struct func_type_MaaIsImageEmpty {
  struct _0_handle {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBuffer *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = unsigned char;
  };
  using args = std::tuple<_0_handle, _ret>;
};
struct function_MaaIsImageEmpty {
  constexpr static auto func = MaaIsImageEmpty;
  using type = func_type_MaaIsImageEmpty;
};

struct func_type_MaaClearImage {
  struct _0_handle {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBuffer *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = unsigned char;
  };
  using args = std::tuple<_0_handle, _ret>;
};
struct function_MaaClearImage {
  constexpr static auto func = MaaClearImage;
  using type = func_type_MaaClearImage;
};

struct func_type_MaaGetImageWidth {
  struct _0_handle {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBuffer *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = int;
  };
  using args = std::tuple<_0_handle, _ret>;
};
struct function_MaaGetImageWidth {
  constexpr static auto func = MaaGetImageWidth;
  using type = func_type_MaaGetImageWidth;
};

struct func_type_MaaGetImageHeight {
  struct _0_handle {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBuffer *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = int;
  };
  using args = std::tuple<_0_handle, _ret>;
};
struct function_MaaGetImageHeight {
  constexpr static auto func = MaaGetImageHeight;
  using type = func_type_MaaGetImageHeight;
};

struct func_type_MaaGetImageType {
  struct _0_handle {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBuffer *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = int;
  };
  using args = std::tuple<_0_handle, _ret>;
};
struct function_MaaGetImageType {
  constexpr static auto func = MaaGetImageType;
  using type = func_type_MaaGetImageType;
};

struct func_type_MaaGetImageEncoded {
  struct _0_handle {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBuffer *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = unsigned char *;
  };
  using args = std::tuple<_0_handle, _ret>;
};
struct function_MaaGetImageEncoded {
  constexpr static auto func = MaaGetImageEncoded;
  using type = func_type_MaaGetImageEncoded;
};

struct func_type_MaaSetImageEncoded {
  struct _0_handle {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBuffer *;
  };
  struct _1_data {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "data";
    using type = unsigned char *;
  };
  struct _2_size {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "size";
    using type = unsigned long long;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 3;
    using type = unsigned char;
  };
  using args = std::tuple<_0_handle, _1_data, _2_size, _ret>;
};
struct function_MaaSetImageEncoded {
  constexpr static auto func = MaaSetImageEncoded;
  using type = func_type_MaaSetImageEncoded;
};

struct func_type_MaaVersion {
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 0;
    using type = const char *;
  };
  using args = std::tuple<_ret>;
};
struct function_MaaVersion {
  constexpr static auto func = MaaVersion;
  using type = func_type_MaaVersion;
};

struct func_type_MaaToolkitInit {
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 0;
    using type = unsigned char;
  };
  using args = std::tuple<_ret>;
};
struct function_MaaToolkitInit {
  constexpr static auto func = MaaToolkitInit;
  using type = func_type_MaaToolkitInit;
};

struct func_type_MaaToolkitUninit {
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 0;
    using type = unsigned char;
  };
  using args = std::tuple<_ret>;
};
struct function_MaaToolkitUninit {
  constexpr static auto func = MaaToolkitUninit;
  using type = func_type_MaaToolkitUninit;
};

struct func_type_MaaToolkitFindDevice {
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 0;
    using type = unsigned long long;
  };
  using args = std::tuple<_ret>;
};
struct function_MaaToolkitFindDevice {
  constexpr static auto func = MaaToolkitFindDevice;
  using type = func_type_MaaToolkitFindDevice;
};

struct func_type_MaaToolkitFindDeviceWithAdb {
  struct _0_adb_path {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "adb_path";
    using type = const char *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = unsigned long long;
  };
  using args = std::tuple<_0_adb_path, _ret>;
};
struct function_MaaToolkitFindDeviceWithAdb {
  constexpr static auto func = MaaToolkitFindDeviceWithAdb;
  using type = func_type_MaaToolkitFindDeviceWithAdb;
};

struct func_type_MaaToolkitPostFindDevice {
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 0;
    using type = unsigned char;
  };
  using args = std::tuple<_ret>;
};
struct function_MaaToolkitPostFindDevice {
  constexpr static auto func = MaaToolkitPostFindDevice;
  using type = func_type_MaaToolkitPostFindDevice;
};

struct func_type_MaaToolkitPostFindDeviceWithAdb {
  struct _0_adb_path {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "adb_path";
    using type = const char *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = unsigned char;
  };
  using args = std::tuple<_0_adb_path, _ret>;
};
struct function_MaaToolkitPostFindDeviceWithAdb {
  constexpr static auto func = MaaToolkitPostFindDeviceWithAdb;
  using type = func_type_MaaToolkitPostFindDeviceWithAdb;
};

struct func_type_MaaToolkitIsFindDeviceCompleted {
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 0;
    using type = unsigned char;
  };
  using args = std::tuple<_ret>;
};
struct function_MaaToolkitIsFindDeviceCompleted {
  constexpr static auto func = MaaToolkitIsFindDeviceCompleted;
  using type = func_type_MaaToolkitIsFindDeviceCompleted;
};

struct func_type_MaaToolkitWaitForFindDeviceToComplete {
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 0;
    using type = unsigned long long;
  };
  using args = std::tuple<_ret>;
};
struct function_MaaToolkitWaitForFindDeviceToComplete {
  constexpr static auto func = MaaToolkitWaitForFindDeviceToComplete;
  using type = func_type_MaaToolkitWaitForFindDeviceToComplete;
};

struct func_type_MaaToolkitGetDeviceCount {
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 0;
    using type = unsigned long long;
  };
  using args = std::tuple<_ret>;
};
struct function_MaaToolkitGetDeviceCount {
  constexpr static auto func = MaaToolkitGetDeviceCount;
  using type = func_type_MaaToolkitGetDeviceCount;
};

struct func_type_MaaToolkitGetDeviceName {
  struct _0_index {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = unsigned long long;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = const char *;
  };
  using args = std::tuple<_0_index, _ret>;
};
struct function_MaaToolkitGetDeviceName {
  constexpr static auto func = MaaToolkitGetDeviceName;
  using type = func_type_MaaToolkitGetDeviceName;
};

struct func_type_MaaToolkitGetDeviceAdbPath {
  struct _0_index {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = unsigned long long;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = const char *;
  };
  using args = std::tuple<_0_index, _ret>;
};
struct function_MaaToolkitGetDeviceAdbPath {
  constexpr static auto func = MaaToolkitGetDeviceAdbPath;
  using type = func_type_MaaToolkitGetDeviceAdbPath;
};

struct func_type_MaaToolkitGetDeviceAdbSerial {
  struct _0_index {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = unsigned long long;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = const char *;
  };
  using args = std::tuple<_0_index, _ret>;
};
struct function_MaaToolkitGetDeviceAdbSerial {
  constexpr static auto func = MaaToolkitGetDeviceAdbSerial;
  using type = func_type_MaaToolkitGetDeviceAdbSerial;
};

struct func_type_MaaToolkitGetDeviceAdbControllerType {
  struct _0_index {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = unsigned long long;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = int;
  };
  using args = std::tuple<_0_index, _ret>;
};
struct function_MaaToolkitGetDeviceAdbControllerType {
  constexpr static auto func = MaaToolkitGetDeviceAdbControllerType;
  using type = func_type_MaaToolkitGetDeviceAdbControllerType;
};

struct func_type_MaaToolkitGetDeviceAdbConfig {
  struct _0_index {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = unsigned long long;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = const char *;
  };
  using args = std::tuple<_0_index, _ret>;
};
struct function_MaaToolkitGetDeviceAdbConfig {
  constexpr static auto func = MaaToolkitGetDeviceAdbConfig;
  using type = func_type_MaaToolkitGetDeviceAdbConfig;
};

struct func_type_MaaToolkitRegisterCustomRecognizerExecutor {
  struct _0_handle {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaInstanceAPI *;
  };
  struct _1_recognizer_name {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "recognizer_name";
    using type = const char *;
  };
  struct _2_recognizer_exec_path {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "recognizer_exec_path";
    using type = const char *;
  };
  struct _3_recognizer_exec_param_json {
    constexpr static bool ret = false;
    constexpr static size_t index = 3;
    constexpr static const char* name = "recognizer_exec_param_json";
    using type = const char *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 4;
    using type = unsigned char;
  };
  using args = std::tuple<_0_handle, _1_recognizer_name, _2_recognizer_exec_path, _3_recognizer_exec_param_json, _ret>;
};
struct function_MaaToolkitRegisterCustomRecognizerExecutor {
  constexpr static auto func = MaaToolkitRegisterCustomRecognizerExecutor;
  using type = func_type_MaaToolkitRegisterCustomRecognizerExecutor;
};

struct func_type_MaaToolkitUnregisterCustomRecognizerExecutor {
  struct _0_handle {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaInstanceAPI *;
  };
  struct _1_recognizer_name {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "recognizer_name";
    using type = const char *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = unsigned char;
  };
  using args = std::tuple<_0_handle, _1_recognizer_name, _ret>;
};
struct function_MaaToolkitUnregisterCustomRecognizerExecutor {
  constexpr static auto func = MaaToolkitUnregisterCustomRecognizerExecutor;
  using type = func_type_MaaToolkitUnregisterCustomRecognizerExecutor;
};

struct func_type_MaaToolkitRegisterCustomActionExecutor {
  struct _0_handle {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaInstanceAPI *;
  };
  struct _1_action_name {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "action_name";
    using type = const char *;
  };
  struct _2_action_exec_path {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "action_exec_path";
    using type = const char *;
  };
  struct _3_action_exec_param_json {
    constexpr static bool ret = false;
    constexpr static size_t index = 3;
    constexpr static const char* name = "action_exec_param_json";
    using type = const char *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 4;
    using type = unsigned char;
  };
  using args = std::tuple<_0_handle, _1_action_name, _2_action_exec_path, _3_action_exec_param_json, _ret>;
};
struct function_MaaToolkitRegisterCustomActionExecutor {
  constexpr static auto func = MaaToolkitRegisterCustomActionExecutor;
  using type = func_type_MaaToolkitRegisterCustomActionExecutor;
};

struct func_type_MaaToolkitUnregisterCustomActionExecutor {
  struct _0_handle {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaInstanceAPI *;
  };
  struct _1_action_name {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "action_name";
    using type = const char *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = unsigned char;
  };
  using args = std::tuple<_0_handle, _1_action_name, _ret>;
};
struct function_MaaToolkitUnregisterCustomActionExecutor {
  constexpr static auto func = MaaToolkitUnregisterCustomActionExecutor;
  using type = func_type_MaaToolkitUnregisterCustomActionExecutor;
};

struct func_type_MaaToolkitFindWindow {
  struct _0_class_name {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "class_name";
    using type = const char *;
  };
  struct _1_window_name {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "window_name";
    using type = const char *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = unsigned long long;
  };
  using args = std::tuple<_0_class_name, _1_window_name, _ret>;
};
struct function_MaaToolkitFindWindow {
  constexpr static auto func = MaaToolkitFindWindow;
  using type = func_type_MaaToolkitFindWindow;
};

struct func_type_MaaToolkitSearchWindow {
  struct _0_class_name {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "class_name";
    using type = const char *;
  };
  struct _1_window_name {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "window_name";
    using type = const char *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = unsigned long long;
  };
  using args = std::tuple<_0_class_name, _1_window_name, _ret>;
};
struct function_MaaToolkitSearchWindow {
  constexpr static auto func = MaaToolkitSearchWindow;
  using type = func_type_MaaToolkitSearchWindow;
};

struct func_type_MaaToolkitGetWindow {
  struct _0_index {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = unsigned long long;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 1;
    using type = unsigned long long;
  };
  using args = std::tuple<_0_index, _ret>;
};
struct function_MaaToolkitGetWindow {
  constexpr static auto func = MaaToolkitGetWindow;
  using type = func_type_MaaToolkitGetWindow;
};

struct func_type_MaaToolkitGetCursorWindow {
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 0;
    using type = unsigned long long;
  };
  using args = std::tuple<_ret>;
};
struct function_MaaToolkitGetCursorWindow {
  constexpr static auto func = MaaToolkitGetCursorWindow;
  using type = func_type_MaaToolkitGetCursorWindow;
};

struct func_type_MaaToolkitGetDesktopWindow {
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 0;
    using type = unsigned long long;
  };
  using args = std::tuple<_ret>;
};
struct function_MaaToolkitGetDesktopWindow {
  constexpr static auto func = MaaToolkitGetDesktopWindow;
  using type = func_type_MaaToolkitGetDesktopWindow;
};

struct func_type_MaaToolkitGetForegroundWindow {
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 0;
    using type = unsigned long long;
  };
  using args = std::tuple<_ret>;
};
struct function_MaaToolkitGetForegroundWindow {
  constexpr static auto func = MaaToolkitGetForegroundWindow;
  using type = func_type_MaaToolkitGetForegroundWindow;
};

struct func_type_MaaControllerSetOptionString {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_key {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "key";
    using type = int;
  };
  struct _2_value {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "value";
    using type = const char *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 3;
    using type = unsigned char;
  };
  using args = std::tuple<_0_ctrl, _1_key, _2_value, _ret>;
};
struct function_MaaControllerSetOptionString {
  constexpr static auto func = MaaControllerSetOptionString;
  using type = func_type_MaaControllerSetOptionString;
};

struct func_type_MaaControllerSetOptionInteger {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_key {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "key";
    using type = int;
  };
  struct _2_value {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "value";
    using type = int;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 3;
    using type = unsigned char;
  };
  using args = std::tuple<_0_ctrl, _1_key, _2_value, _ret>;
};
struct function_MaaControllerSetOptionInteger {
  constexpr static auto func = MaaControllerSetOptionInteger;
  using type = func_type_MaaControllerSetOptionInteger;
};

struct func_type_MaaControllerSetOptionBoolean {
  struct _0_ctrl {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_key {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "key";
    using type = int;
  };
  struct _2_value {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "value";
    using type = bool;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 3;
    using type = unsigned char;
  };
  using args = std::tuple<_0_ctrl, _1_key, _2_value, _ret>;
};
struct function_MaaControllerSetOptionBoolean {
  constexpr static auto func = MaaControllerSetOptionBoolean;
  using type = func_type_MaaControllerSetOptionBoolean;
};

struct func_type_MaaSetGlobalOptionString {
  struct _0_key {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "key";
    using type = int;
  };
  struct _1_value {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "value";
    using type = const char *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = unsigned char;
  };
  using args = std::tuple<_0_key, _1_value, _ret>;
};
struct function_MaaSetGlobalOptionString {
  constexpr static auto func = MaaSetGlobalOptionString;
  using type = func_type_MaaSetGlobalOptionString;
};

struct func_type_MaaSetGlobalOptionInteger {
  struct _0_key {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "key";
    using type = int;
  };
  struct _1_value {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "value";
    using type = int;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = unsigned char;
  };
  using args = std::tuple<_0_key, _1_value, _ret>;
};
struct function_MaaSetGlobalOptionInteger {
  constexpr static auto func = MaaSetGlobalOptionInteger;
  using type = func_type_MaaSetGlobalOptionInteger;
};

struct func_type_MaaSetGlobalOptionBoolean {
  struct _0_key {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "key";
    using type = int;
  };
  struct _1_value {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "value";
    using type = bool;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 2;
    using type = unsigned char;
  };
  using args = std::tuple<_0_key, _1_value, _ret>;
};
struct function_MaaSetGlobalOptionBoolean {
  constexpr static auto func = MaaSetGlobalOptionBoolean;
  using type = func_type_MaaSetGlobalOptionBoolean;
};

struct func_type_MaaRegisterCustomRecognizerImpl {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_name {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "name";
    using type = const char *;
  };
  struct _2_analyze {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "analyze";
    using type = unsigned char (*)(MaaSyncContextAPI *, MaaImageBuffer *, const char *, const char *, void *, MaaRect *, MaaStringBuffer *);
  };
  struct _3_analyze_arg {
    constexpr static bool ret = false;
    constexpr static size_t index = 3;
    constexpr static const char* name = "analyze_arg";
    using type = void *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 4;
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _1_name, _2_analyze, _3_analyze_arg, _ret>;
};
struct function_MaaRegisterCustomRecognizerImpl {
  constexpr static auto func = MaaRegisterCustomRecognizerImpl;
  using type = func_type_MaaRegisterCustomRecognizerImpl;
};

struct func_type_MaaRegisterCustomActionImpl {
  struct _0_inst {
    constexpr static bool ret = false;
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_name {
    constexpr static bool ret = false;
    constexpr static size_t index = 1;
    constexpr static const char* name = "name";
    using type = const char *;
  };
  struct _2_run {
    constexpr static bool ret = false;
    constexpr static size_t index = 2;
    constexpr static const char* name = "run";
    using type = unsigned char (*)(MaaSyncContextAPI *, const char *, const char *, MaaRect *, const char *, void *);
  };
  struct _3_run_arg {
    constexpr static bool ret = false;
    constexpr static size_t index = 3;
    constexpr static const char* name = "run_arg";
    using type = void *;
  };
  struct _4_stop {
    constexpr static bool ret = false;
    constexpr static size_t index = 4;
    constexpr static const char* name = "stop";
    using type = void (*)(void *);
  };
  struct _5_stop_arg {
    constexpr static bool ret = false;
    constexpr static size_t index = 5;
    constexpr static const char* name = "stop_arg";
    using type = void *;
  };
  struct _ret {
    constexpr static bool ret = true;
    constexpr static size_t index = 6;
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _1_name, _2_run, _3_run_arg, _4_stop, _5_stop_arg, _ret>;
};
struct function_MaaRegisterCustomActionImpl {
  constexpr static auto func = MaaRegisterCustomActionImpl;
  using type = func_type_MaaRegisterCustomActionImpl;
};


}

template <>
struct is_input<maa::func_type_MaaAdbControllerCreate::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaAdbControllerCreate::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaAdbControllerCreate::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaAdbControllerCreateV2::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaAdbControllerCreateV2::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaAdbControllerCreateV2::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaThriftControllerCreate::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaThriftControllerCreate::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaThriftControllerCreate::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaDbgControllerCreate::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaDbgControllerCreate::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaDbgControllerCreate::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerDestroy::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerDestroy::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerDestroy::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerPostConnection::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerPostConnection::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerPostConnection::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerPostClick::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerPostClick::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerPostClick::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerPostSwipe::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerPostSwipe::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerPostSwipe::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerPostPressKey::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerPostPressKey::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerPostPressKey::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerPostInputText::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerPostInputText::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerPostInputText::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerPostTouchDown::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerPostTouchDown::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerPostTouchDown::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerPostTouchMove::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerPostTouchMove::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerPostTouchMove::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerPostTouchUp::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerPostTouchUp::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerPostTouchUp::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerPostScreencap::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerPostScreencap::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerPostScreencap::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerStatus::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerStatus::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerStatus::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerWait::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerWait::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerWait::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerConnected::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerConnected::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerConnected::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerGetImage::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerGetImage::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerGetImage::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerGetUUID::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerGetUUID::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerGetUUID::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaCreate::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaCreate::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaCreate::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaDestroy::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaDestroy::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaDestroy::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaBindResource::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaBindResource::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaBindResource::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaBindController::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaBindController::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaBindController::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaInited::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaInited::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaInited::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaUnregisterCustomRecognizer::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaUnregisterCustomRecognizer::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaUnregisterCustomRecognizer::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaClearCustomRecognizer::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaClearCustomRecognizer::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaClearCustomRecognizer::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaUnregisterCustomAction::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaUnregisterCustomAction::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaUnregisterCustomAction::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaClearCustomAction::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaClearCustomAction::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaClearCustomAction::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaPostTask::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaPostTask::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaPostTask::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaSetTaskParam::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaSetTaskParam::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaSetTaskParam::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaTaskStatus::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaTaskStatus::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaTaskStatus::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaWaitTask::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaWaitTask::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaWaitTask::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaTaskAllFinished::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaTaskAllFinished::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaTaskAllFinished::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaPostStop::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaPostStop::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaPostStop::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaStop::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaStop::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaStop::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaGetResource::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaGetResource::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaGetResource::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaGetController::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaGetController::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaGetController::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaResourceCreate::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaResourceCreate::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaResourceCreate::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaResourceDestroy::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaResourceDestroy::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaResourceDestroy::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaResourcePostPath::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaResourcePostPath::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaResourcePostPath::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaResourceStatus::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaResourceStatus::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaResourceStatus::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaResourceWait::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaResourceWait::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaResourceWait::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaResourceLoaded::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaResourceLoaded::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaResourceLoaded::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaResourceGetHash::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaResourceGetHash::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaResourceGetHash::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaResourceGetTaskList::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaResourceGetTaskList::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaResourceGetTaskList::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaCreateImageBuffer::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaCreateImageBuffer::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaCreateImageBuffer::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaDestroyImageBuffer::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaDestroyImageBuffer::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaDestroyImageBuffer::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaIsImageEmpty::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaIsImageEmpty::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaIsImageEmpty::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaClearImage::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaClearImage::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaClearImage::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaGetImageWidth::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaGetImageWidth::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaGetImageWidth::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaGetImageHeight::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaGetImageHeight::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaGetImageHeight::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaGetImageType::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaGetImageType::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaGetImageType::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaGetImageEncoded::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaGetImageEncoded::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaGetImageEncoded::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaSetImageEncoded::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaSetImageEncoded::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaSetImageEncoded::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaVersion::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaVersion::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaVersion::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitInit::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitInit::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitInit::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitUninit::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitUninit::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitUninit::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitFindDevice::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitFindDevice::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitFindDevice::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitFindDeviceWithAdb::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitFindDeviceWithAdb::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitFindDeviceWithAdb::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitPostFindDevice::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitPostFindDevice::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitPostFindDevice::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitPostFindDeviceWithAdb::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitPostFindDeviceWithAdb::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitPostFindDeviceWithAdb::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitIsFindDeviceCompleted::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitIsFindDeviceCompleted::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitIsFindDeviceCompleted::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitWaitForFindDeviceToComplete::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitWaitForFindDeviceToComplete::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitWaitForFindDeviceToComplete::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitGetDeviceCount::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitGetDeviceCount::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitGetDeviceCount::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitGetDeviceName::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitGetDeviceName::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitGetDeviceName::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitGetDeviceAdbPath::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitGetDeviceAdbPath::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitGetDeviceAdbPath::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitGetDeviceAdbSerial::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitGetDeviceAdbSerial::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitGetDeviceAdbSerial::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitGetDeviceAdbControllerType::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitGetDeviceAdbControllerType::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitGetDeviceAdbControllerType::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitGetDeviceAdbConfig::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitGetDeviceAdbConfig::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitGetDeviceAdbConfig::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitRegisterCustomRecognizerExecutor::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitRegisterCustomRecognizerExecutor::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitRegisterCustomRecognizerExecutor::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitUnregisterCustomRecognizerExecutor::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitUnregisterCustomRecognizerExecutor::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitUnregisterCustomRecognizerExecutor::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitRegisterCustomActionExecutor::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitRegisterCustomActionExecutor::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitRegisterCustomActionExecutor::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitUnregisterCustomActionExecutor::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitUnregisterCustomActionExecutor::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitUnregisterCustomActionExecutor::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitFindWindow::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitFindWindow::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitFindWindow::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitSearchWindow::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitSearchWindow::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitSearchWindow::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitGetWindow::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitGetWindow::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitGetWindow::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitGetCursorWindow::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitGetCursorWindow::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitGetCursorWindow::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitGetDesktopWindow::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitGetDesktopWindow::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitGetDesktopWindow::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaToolkitGetForegroundWindow::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaToolkitGetForegroundWindow::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaToolkitGetForegroundWindow::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerSetOptionString::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerSetOptionString::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerSetOptionString::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerSetOptionInteger::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerSetOptionInteger::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerSetOptionInteger::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaControllerSetOptionBoolean::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaControllerSetOptionBoolean::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaControllerSetOptionBoolean::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaSetGlobalOptionString::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaSetGlobalOptionString::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaSetGlobalOptionString::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaSetGlobalOptionInteger::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaSetGlobalOptionInteger::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaSetGlobalOptionInteger::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaSetGlobalOptionBoolean::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaSetGlobalOptionBoolean::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaSetGlobalOptionBoolean::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaRegisterCustomRecognizerImpl::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaRegisterCustomRecognizerImpl::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaRegisterCustomRecognizerImpl::_ret::type;
};
template <>
struct is_input<maa::func_type_MaaRegisterCustomActionImpl::_ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template <>
struct is_output<maa::func_type_MaaRegisterCustomActionImpl::_ret, false> {
  constexpr static bool value = true;
  using type = typename maa::func_type_MaaRegisterCustomActionImpl::_ret::type;
};

}