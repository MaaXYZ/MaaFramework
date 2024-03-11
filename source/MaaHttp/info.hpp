#pragma once

#include "function/interface.hpp"

#include "include.h"

namespace lhg {

namespace maa {

struct func_type_MaaAdbControllerCreate {
  struct _0_adb_path {
    constexpr static size_t index = 0;
    constexpr static const char* name = "adb_path";
    using type = const char *;
  };
  struct _1_address {
    constexpr static size_t index = 1;
    constexpr static const char* name = "address";
    using type = const char *;
  };
  struct _2_type {
    constexpr static size_t index = 2;
    constexpr static const char* name = "type";
    using type = int;
  };
  struct _3_config {
    constexpr static size_t index = 3;
    constexpr static const char* name = "config";
    using type = const char *;
  };
  struct _4_callback {
    constexpr static size_t index = 4;
    constexpr static const char* name = "callback";
    using type = void (*)(const char *, const char *, void *);
  };
  struct _5_callback_arg {
    constexpr static size_t index = 5;
    constexpr static const char* name = "callback_arg";
    using type = void *;
  };
  struct ret {
    constexpr static size_t index = 6;
    constexpr static const char* name = "return";
    using type = MaaControllerAPI *;
  };
  using args = std::tuple<_0_adb_path, _1_address, _2_type, _3_config, _4_callback, _5_callback_arg, ret>;
};
struct function_MaaAdbControllerCreate {
  constexpr static auto func = MaaAdbControllerCreate;
  constexpr static const char* name = "MaaAdbControllerCreate";
  using type = func_type_MaaAdbControllerCreate;
};

struct func_type_MaaAdbControllerCreateV2 {
  struct _0_adb_path {
    constexpr static size_t index = 0;
    constexpr static const char* name = "adb_path";
    using type = const char *;
  };
  struct _1_address {
    constexpr static size_t index = 1;
    constexpr static const char* name = "address";
    using type = const char *;
  };
  struct _2_type {
    constexpr static size_t index = 2;
    constexpr static const char* name = "type";
    using type = int;
  };
  struct _3_config {
    constexpr static size_t index = 3;
    constexpr static const char* name = "config";
    using type = const char *;
  };
  struct _4_agent_path {
    constexpr static size_t index = 4;
    constexpr static const char* name = "agent_path";
    using type = const char *;
  };
  struct _5_callback {
    constexpr static size_t index = 5;
    constexpr static const char* name = "callback";
    using type = void (*)(const char *, const char *, void *);
  };
  struct _6_callback_arg {
    constexpr static size_t index = 6;
    constexpr static const char* name = "callback_arg";
    using type = void *;
  };
  struct ret {
    constexpr static size_t index = 7;
    constexpr static const char* name = "return";
    using type = MaaControllerAPI *;
  };
  using args = std::tuple<_0_adb_path, _1_address, _2_type, _3_config, _4_agent_path, _5_callback, _6_callback_arg, ret>;
};
struct function_MaaAdbControllerCreateV2 {
  constexpr static auto func = MaaAdbControllerCreateV2;
  constexpr static const char* name = "MaaAdbControllerCreateV2";
  using type = func_type_MaaAdbControllerCreateV2;
};

struct func_type_MaaThriftControllerCreate {
  struct _0_type {
    constexpr static size_t index = 0;
    constexpr static const char* name = "type";
    using type = int;
  };
  struct _1_host {
    constexpr static size_t index = 1;
    constexpr static const char* name = "host";
    using type = const char *;
  };
  struct _2_port {
    constexpr static size_t index = 2;
    constexpr static const char* name = "port";
    using type = int;
  };
  struct _3_config {
    constexpr static size_t index = 3;
    constexpr static const char* name = "config";
    using type = const char *;
  };
  struct _4_callback {
    constexpr static size_t index = 4;
    constexpr static const char* name = "callback";
    using type = void (*)(const char *, const char *, void *);
  };
  struct _5_callback_arg {
    constexpr static size_t index = 5;
    constexpr static const char* name = "callback_arg";
    using type = void *;
  };
  struct ret {
    constexpr static size_t index = 6;
    constexpr static const char* name = "return";
    using type = MaaControllerAPI *;
  };
  using args = std::tuple<_0_type, _1_host, _2_port, _3_config, _4_callback, _5_callback_arg, ret>;
};
struct function_MaaThriftControllerCreate {
  constexpr static auto func = MaaThriftControllerCreate;
  constexpr static const char* name = "MaaThriftControllerCreate";
  using type = func_type_MaaThriftControllerCreate;
};

struct func_type_MaaDbgControllerCreate {
  struct _0_read_path {
    constexpr static size_t index = 0;
    constexpr static const char* name = "read_path";
    using type = const char *;
  };
  struct _1_write_path {
    constexpr static size_t index = 1;
    constexpr static const char* name = "write_path";
    using type = const char *;
  };
  struct _2_type {
    constexpr static size_t index = 2;
    constexpr static const char* name = "type";
    using type = int;
  };
  struct _3_config {
    constexpr static size_t index = 3;
    constexpr static const char* name = "config";
    using type = const char *;
  };
  struct _4_callback {
    constexpr static size_t index = 4;
    constexpr static const char* name = "callback";
    using type = void (*)(const char *, const char *, void *);
  };
  struct _5_callback_arg {
    constexpr static size_t index = 5;
    constexpr static const char* name = "callback_arg";
    using type = void *;
  };
  struct ret {
    constexpr static size_t index = 6;
    constexpr static const char* name = "return";
    using type = MaaControllerAPI *;
  };
  using args = std::tuple<_0_read_path, _1_write_path, _2_type, _3_config, _4_callback, _5_callback_arg, ret>;
};
struct function_MaaDbgControllerCreate {
  constexpr static auto func = MaaDbgControllerCreate;
  constexpr static const char* name = "MaaDbgControllerCreate";
  using type = func_type_MaaDbgControllerCreate;
};

struct func_type_MaaControllerDestroy {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
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

struct func_type_MaaControllerPostConnection {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, ret>;
};
struct function_MaaControllerPostConnection {
  constexpr static auto func = MaaControllerPostConnection;
  constexpr static const char* name = "MaaControllerPostConnection";
  using type = func_type_MaaControllerPostConnection;
};

struct func_type_MaaControllerPostClick {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_x {
    constexpr static size_t index = 1;
    constexpr static const char* name = "x";
    using type = int;
  };
  struct _2_y {
    constexpr static size_t index = 2;
    constexpr static const char* name = "y";
    using type = int;
  };
  struct ret {
    constexpr static size_t index = 3;
    constexpr static const char* name = "return";
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, _1_x, _2_y, ret>;
};
struct function_MaaControllerPostClick {
  constexpr static auto func = MaaControllerPostClick;
  constexpr static const char* name = "MaaControllerPostClick";
  using type = func_type_MaaControllerPostClick;
};

struct func_type_MaaControllerPostSwipe {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_x1 {
    constexpr static size_t index = 1;
    constexpr static const char* name = "x1";
    using type = int;
  };
  struct _2_y1 {
    constexpr static size_t index = 2;
    constexpr static const char* name = "y1";
    using type = int;
  };
  struct _3_x2 {
    constexpr static size_t index = 3;
    constexpr static const char* name = "x2";
    using type = int;
  };
  struct _4_y2 {
    constexpr static size_t index = 4;
    constexpr static const char* name = "y2";
    using type = int;
  };
  struct _5_duration {
    constexpr static size_t index = 5;
    constexpr static const char* name = "duration";
    using type = int;
  };
  struct ret {
    constexpr static size_t index = 6;
    constexpr static const char* name = "return";
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, _1_x1, _2_y1, _3_x2, _4_y2, _5_duration, ret>;
};
struct function_MaaControllerPostSwipe {
  constexpr static auto func = MaaControllerPostSwipe;
  constexpr static const char* name = "MaaControllerPostSwipe";
  using type = func_type_MaaControllerPostSwipe;
};

struct func_type_MaaControllerPostPressKey {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_keycode {
    constexpr static size_t index = 1;
    constexpr static const char* name = "keycode";
    using type = int;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, _1_keycode, ret>;
};
struct function_MaaControllerPostPressKey {
  constexpr static auto func = MaaControllerPostPressKey;
  constexpr static const char* name = "MaaControllerPostPressKey";
  using type = func_type_MaaControllerPostPressKey;
};

struct func_type_MaaControllerPostInputText {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_text {
    constexpr static size_t index = 1;
    constexpr static const char* name = "text";
    using type = const char *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, _1_text, ret>;
};
struct function_MaaControllerPostInputText {
  constexpr static auto func = MaaControllerPostInputText;
  constexpr static const char* name = "MaaControllerPostInputText";
  using type = func_type_MaaControllerPostInputText;
};

struct func_type_MaaControllerPostTouchDown {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_contact {
    constexpr static size_t index = 1;
    constexpr static const char* name = "contact";
    using type = int;
  };
  struct _2_x {
    constexpr static size_t index = 2;
    constexpr static const char* name = "x";
    using type = int;
  };
  struct _3_y {
    constexpr static size_t index = 3;
    constexpr static const char* name = "y";
    using type = int;
  };
  struct _4_pressure {
    constexpr static size_t index = 4;
    constexpr static const char* name = "pressure";
    using type = int;
  };
  struct ret {
    constexpr static size_t index = 5;
    constexpr static const char* name = "return";
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, _1_contact, _2_x, _3_y, _4_pressure, ret>;
};
struct function_MaaControllerPostTouchDown {
  constexpr static auto func = MaaControllerPostTouchDown;
  constexpr static const char* name = "MaaControllerPostTouchDown";
  using type = func_type_MaaControllerPostTouchDown;
};

struct func_type_MaaControllerPostTouchMove {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_contact {
    constexpr static size_t index = 1;
    constexpr static const char* name = "contact";
    using type = int;
  };
  struct _2_x {
    constexpr static size_t index = 2;
    constexpr static const char* name = "x";
    using type = int;
  };
  struct _3_y {
    constexpr static size_t index = 3;
    constexpr static const char* name = "y";
    using type = int;
  };
  struct _4_pressure {
    constexpr static size_t index = 4;
    constexpr static const char* name = "pressure";
    using type = int;
  };
  struct ret {
    constexpr static size_t index = 5;
    constexpr static const char* name = "return";
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, _1_contact, _2_x, _3_y, _4_pressure, ret>;
};
struct function_MaaControllerPostTouchMove {
  constexpr static auto func = MaaControllerPostTouchMove;
  constexpr static const char* name = "MaaControllerPostTouchMove";
  using type = func_type_MaaControllerPostTouchMove;
};

struct func_type_MaaControllerPostTouchUp {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_contact {
    constexpr static size_t index = 1;
    constexpr static const char* name = "contact";
    using type = int;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, _1_contact, ret>;
};
struct function_MaaControllerPostTouchUp {
  constexpr static auto func = MaaControllerPostTouchUp;
  constexpr static const char* name = "MaaControllerPostTouchUp";
  using type = func_type_MaaControllerPostTouchUp;
};

struct func_type_MaaControllerPostScreencap {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = long long;
  };
  using args = std::tuple<_0_ctrl, ret>;
};
struct function_MaaControllerPostScreencap {
  constexpr static auto func = MaaControllerPostScreencap;
  constexpr static const char* name = "MaaControllerPostScreencap";
  using type = func_type_MaaControllerPostScreencap;
};

struct func_type_MaaControllerStatus {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_id {
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = long long;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = int;
  };
  using args = std::tuple<_0_ctrl, _1_id, ret>;
};
struct function_MaaControllerStatus {
  constexpr static auto func = MaaControllerStatus;
  constexpr static const char* name = "MaaControllerStatus";
  using type = func_type_MaaControllerStatus;
};

struct func_type_MaaControllerWait {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_id {
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = long long;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = int;
  };
  using args = std::tuple<_0_ctrl, _1_id, ret>;
};
struct function_MaaControllerWait {
  constexpr static auto func = MaaControllerWait;
  constexpr static const char* name = "MaaControllerWait";
  using type = func_type_MaaControllerWait;
};

struct func_type_MaaControllerConnected {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_ctrl, ret>;
};
struct function_MaaControllerConnected {
  constexpr static auto func = MaaControllerConnected;
  constexpr static const char* name = "MaaControllerConnected";
  using type = func_type_MaaControllerConnected;
};

struct func_type_MaaControllerGetImage {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_buffer {
    constexpr static size_t index = 1;
    constexpr static const char* name = "buffer";
    using type = MaaImageBuffer *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_ctrl, _1_buffer, ret>;
};
struct function_MaaControllerGetImage {
  constexpr static auto func = MaaControllerGetImage;
  constexpr static const char* name = "MaaControllerGetImage";
  using type = func_type_MaaControllerGetImage;
};

struct func_type_MaaControllerGetUUID {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_buffer {
    constexpr static size_t index = 1;
    constexpr static const char* name = "buffer";
    using type = MaaStringBuffer *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_ctrl, _1_buffer, ret>;
};
struct function_MaaControllerGetUUID {
  constexpr static auto func = MaaControllerGetUUID;
  constexpr static const char* name = "MaaControllerGetUUID";
  using type = func_type_MaaControllerGetUUID;
};

struct func_type_MaaCreate {
  struct _0_callback {
    constexpr static size_t index = 0;
    constexpr static const char* name = "callback";
    using type = void (*)(const char *, const char *, void *);
  };
  struct _1_callback_arg {
    constexpr static size_t index = 1;
    constexpr static const char* name = "callback_arg";
    using type = void *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaInstanceAPI *;
  };
  using args = std::tuple<_0_callback, _1_callback_arg, ret>;
};
struct function_MaaCreate {
  constexpr static auto func = MaaCreate;
  constexpr static const char* name = "MaaCreate";
  using type = func_type_MaaCreate;
};

struct func_type_MaaDestroy {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
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

struct func_type_MaaBindResource {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_res {
    constexpr static size_t index = 1;
    constexpr static const char* name = "res";
    using type = MaaResourceAPI *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _1_res, ret>;
};
struct function_MaaBindResource {
  constexpr static auto func = MaaBindResource;
  constexpr static const char* name = "MaaBindResource";
  using type = func_type_MaaBindResource;
};

struct func_type_MaaBindController {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_ctrl {
    constexpr static size_t index = 1;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _1_ctrl, ret>;
};
struct function_MaaBindController {
  constexpr static auto func = MaaBindController;
  constexpr static const char* name = "MaaBindController";
  using type = func_type_MaaBindController;
};

struct func_type_MaaInited {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaInited {
  constexpr static auto func = MaaInited;
  constexpr static const char* name = "MaaInited";
  using type = func_type_MaaInited;
};

struct func_type_MaaUnregisterCustomRecognizer {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "name";
    using type = const char *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _1_name, ret>;
};
struct function_MaaUnregisterCustomRecognizer {
  constexpr static auto func = MaaUnregisterCustomRecognizer;
  constexpr static const char* name = "MaaUnregisterCustomRecognizer";
  using type = func_type_MaaUnregisterCustomRecognizer;
};

struct func_type_MaaClearCustomRecognizer {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaClearCustomRecognizer {
  constexpr static auto func = MaaClearCustomRecognizer;
  constexpr static const char* name = "MaaClearCustomRecognizer";
  using type = func_type_MaaClearCustomRecognizer;
};

struct func_type_MaaUnregisterCustomAction {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "name";
    using type = const char *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _1_name, ret>;
};
struct function_MaaUnregisterCustomAction {
  constexpr static auto func = MaaUnregisterCustomAction;
  constexpr static const char* name = "MaaUnregisterCustomAction";
  using type = func_type_MaaUnregisterCustomAction;
};

struct func_type_MaaClearCustomAction {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaClearCustomAction {
  constexpr static auto func = MaaClearCustomAction;
  constexpr static const char* name = "MaaClearCustomAction";
  using type = func_type_MaaClearCustomAction;
};

struct func_type_MaaPostTask {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_entry {
    constexpr static size_t index = 1;
    constexpr static const char* name = "entry";
    using type = const char *;
  };
  struct _2_param {
    constexpr static size_t index = 2;
    constexpr static const char* name = "param";
    using type = const char *;
  };
  struct ret {
    constexpr static size_t index = 3;
    constexpr static const char* name = "return";
    using type = long long;
  };
  using args = std::tuple<_0_inst, _1_entry, _2_param, ret>;
};
struct function_MaaPostTask {
  constexpr static auto func = MaaPostTask;
  constexpr static const char* name = "MaaPostTask";
  using type = func_type_MaaPostTask;
};

struct func_type_MaaSetTaskParam {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_id {
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = long long;
  };
  struct _2_param {
    constexpr static size_t index = 2;
    constexpr static const char* name = "param";
    using type = const char *;
  };
  struct ret {
    constexpr static size_t index = 3;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _1_id, _2_param, ret>;
};
struct function_MaaSetTaskParam {
  constexpr static auto func = MaaSetTaskParam;
  constexpr static const char* name = "MaaSetTaskParam";
  using type = func_type_MaaSetTaskParam;
};

struct func_type_MaaTaskStatus {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_id {
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = long long;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = int;
  };
  using args = std::tuple<_0_inst, _1_id, ret>;
};
struct function_MaaTaskStatus {
  constexpr static auto func = MaaTaskStatus;
  constexpr static const char* name = "MaaTaskStatus";
  using type = func_type_MaaTaskStatus;
};

struct func_type_MaaWaitTask {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_id {
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = long long;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = int;
  };
  using args = std::tuple<_0_inst, _1_id, ret>;
};
struct function_MaaWaitTask {
  constexpr static auto func = MaaWaitTask;
  constexpr static const char* name = "MaaWaitTask";
  using type = func_type_MaaWaitTask;
};

struct func_type_MaaTaskAllFinished {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaTaskAllFinished {
  constexpr static auto func = MaaTaskAllFinished;
  constexpr static const char* name = "MaaTaskAllFinished";
  using type = func_type_MaaTaskAllFinished;
};

struct func_type_MaaPostStop {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaPostStop {
  constexpr static auto func = MaaPostStop;
  constexpr static const char* name = "MaaPostStop";
  using type = func_type_MaaPostStop;
};

struct func_type_MaaStop {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaStop {
  constexpr static auto func = MaaStop;
  constexpr static const char* name = "MaaStop";
  using type = func_type_MaaStop;
};

struct func_type_MaaGetResource {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaResourceAPI *;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaGetResource {
  constexpr static auto func = MaaGetResource;
  constexpr static const char* name = "MaaGetResource";
  using type = func_type_MaaGetResource;
};

struct func_type_MaaGetController {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = MaaControllerAPI *;
  };
  using args = std::tuple<_0_inst, ret>;
};
struct function_MaaGetController {
  constexpr static auto func = MaaGetController;
  constexpr static const char* name = "MaaGetController";
  using type = func_type_MaaGetController;
};

struct func_type_MaaResourceCreate {
  struct _0_callback {
    constexpr static size_t index = 0;
    constexpr static const char* name = "callback";
    using type = void (*)(const char *, const char *, void *);
  };
  struct _1_callback_arg {
    constexpr static size_t index = 1;
    constexpr static const char* name = "callback_arg";
    using type = void *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = MaaResourceAPI *;
  };
  using args = std::tuple<_0_callback, _1_callback_arg, ret>;
};
struct function_MaaResourceCreate {
  constexpr static auto func = MaaResourceCreate;
  constexpr static const char* name = "MaaResourceCreate";
  using type = func_type_MaaResourceCreate;
};

struct func_type_MaaResourceDestroy {
  struct _0_res {
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceAPI *;
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

struct func_type_MaaResourcePostPath {
  struct _0_res {
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceAPI *;
  };
  struct _1_path {
    constexpr static size_t index = 1;
    constexpr static const char* name = "path";
    using type = const char *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = long long;
  };
  using args = std::tuple<_0_res, _1_path, ret>;
};
struct function_MaaResourcePostPath {
  constexpr static auto func = MaaResourcePostPath;
  constexpr static const char* name = "MaaResourcePostPath";
  using type = func_type_MaaResourcePostPath;
};

struct func_type_MaaResourceStatus {
  struct _0_res {
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceAPI *;
  };
  struct _1_id {
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = long long;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = int;
  };
  using args = std::tuple<_0_res, _1_id, ret>;
};
struct function_MaaResourceStatus {
  constexpr static auto func = MaaResourceStatus;
  constexpr static const char* name = "MaaResourceStatus";
  using type = func_type_MaaResourceStatus;
};

struct func_type_MaaResourceWait {
  struct _0_res {
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceAPI *;
  };
  struct _1_id {
    constexpr static size_t index = 1;
    constexpr static const char* name = "id";
    using type = long long;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = int;
  };
  using args = std::tuple<_0_res, _1_id, ret>;
};
struct function_MaaResourceWait {
  constexpr static auto func = MaaResourceWait;
  constexpr static const char* name = "MaaResourceWait";
  using type = func_type_MaaResourceWait;
};

struct func_type_MaaResourceLoaded {
  struct _0_res {
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceAPI *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_res, ret>;
};
struct function_MaaResourceLoaded {
  constexpr static auto func = MaaResourceLoaded;
  constexpr static const char* name = "MaaResourceLoaded";
  using type = func_type_MaaResourceLoaded;
};

struct func_type_MaaResourceGetHash {
  struct _0_res {
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceAPI *;
  };
  struct _1_buffer {
    constexpr static size_t index = 1;
    constexpr static const char* name = "buffer";
    using type = MaaStringBuffer *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_res, _1_buffer, ret>;
};
struct function_MaaResourceGetHash {
  constexpr static auto func = MaaResourceGetHash;
  constexpr static const char* name = "MaaResourceGetHash";
  using type = func_type_MaaResourceGetHash;
};

struct func_type_MaaResourceGetTaskList {
  struct _0_res {
    constexpr static size_t index = 0;
    constexpr static const char* name = "res";
    using type = MaaResourceAPI *;
  };
  struct _1_buffer {
    constexpr static size_t index = 1;
    constexpr static const char* name = "buffer";
    using type = MaaStringBuffer *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_res, _1_buffer, ret>;
};
struct function_MaaResourceGetTaskList {
  constexpr static auto func = MaaResourceGetTaskList;
  constexpr static const char* name = "MaaResourceGetTaskList";
  using type = func_type_MaaResourceGetTaskList;
};

struct func_type_MaaCreateImageBuffer {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = MaaImageBuffer *;
  };
  using args = std::tuple<ret>;
};
struct function_MaaCreateImageBuffer {
  constexpr static auto func = MaaCreateImageBuffer;
  constexpr static const char* name = "MaaCreateImageBuffer";
  using type = func_type_MaaCreateImageBuffer;
};

struct func_type_MaaDestroyImageBuffer {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBuffer *;
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

struct func_type_MaaIsImageEmpty {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBuffer *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_handle, ret>;
};
struct function_MaaIsImageEmpty {
  constexpr static auto func = MaaIsImageEmpty;
  constexpr static const char* name = "MaaIsImageEmpty";
  using type = func_type_MaaIsImageEmpty;
};

struct func_type_MaaClearImage {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBuffer *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_handle, ret>;
};
struct function_MaaClearImage {
  constexpr static auto func = MaaClearImage;
  constexpr static const char* name = "MaaClearImage";
  using type = func_type_MaaClearImage;
};

struct func_type_MaaGetImageWidth {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBuffer *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = int;
  };
  using args = std::tuple<_0_handle, ret>;
};
struct function_MaaGetImageWidth {
  constexpr static auto func = MaaGetImageWidth;
  constexpr static const char* name = "MaaGetImageWidth";
  using type = func_type_MaaGetImageWidth;
};

struct func_type_MaaGetImageHeight {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBuffer *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = int;
  };
  using args = std::tuple<_0_handle, ret>;
};
struct function_MaaGetImageHeight {
  constexpr static auto func = MaaGetImageHeight;
  constexpr static const char* name = "MaaGetImageHeight";
  using type = func_type_MaaGetImageHeight;
};

struct func_type_MaaGetImageType {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBuffer *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = int;
  };
  using args = std::tuple<_0_handle, ret>;
};
struct function_MaaGetImageType {
  constexpr static auto func = MaaGetImageType;
  constexpr static const char* name = "MaaGetImageType";
  using type = func_type_MaaGetImageType;
};

struct func_type_MaaGetImageEncoded {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBuffer *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = unsigned char *;
  };
  using args = std::tuple<_0_handle, ret>;
};
struct function_MaaGetImageEncoded {
  constexpr static auto func = MaaGetImageEncoded;
  constexpr static const char* name = "MaaGetImageEncoded";
  using type = func_type_MaaGetImageEncoded;
};

struct func_type_MaaSetImageEncoded {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaImageBuffer *;
  };
  struct _1_data {
    constexpr static size_t index = 1;
    constexpr static const char* name = "data";
    using type = unsigned char *;
  };
  struct _2_size {
    constexpr static size_t index = 2;
    constexpr static const char* name = "size";
    using type = unsigned long long;
  };
  struct ret {
    constexpr static size_t index = 3;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_handle, _1_data, _2_size, ret>;
};
struct function_MaaSetImageEncoded {
  constexpr static auto func = MaaSetImageEncoded;
  constexpr static const char* name = "MaaSetImageEncoded";
  using type = func_type_MaaSetImageEncoded;
};

struct func_type_MaaVersion {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = const char *;
  };
  using args = std::tuple<ret>;
};
struct function_MaaVersion {
  constexpr static auto func = MaaVersion;
  constexpr static const char* name = "MaaVersion";
  using type = func_type_MaaVersion;
};

struct func_type_MaaToolkitInit {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitInit {
  constexpr static auto func = MaaToolkitInit;
  constexpr static const char* name = "MaaToolkitInit";
  using type = func_type_MaaToolkitInit;
};

struct func_type_MaaToolkitUninit {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitUninit {
  constexpr static auto func = MaaToolkitUninit;
  constexpr static const char* name = "MaaToolkitUninit";
  using type = func_type_MaaToolkitUninit;
};

struct func_type_MaaToolkitFindDevice {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = unsigned long long;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitFindDevice {
  constexpr static auto func = MaaToolkitFindDevice;
  constexpr static const char* name = "MaaToolkitFindDevice";
  using type = func_type_MaaToolkitFindDevice;
};

struct func_type_MaaToolkitFindDeviceWithAdb {
  struct _0_adb_path {
    constexpr static size_t index = 0;
    constexpr static const char* name = "adb_path";
    using type = const char *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = unsigned long long;
  };
  using args = std::tuple<_0_adb_path, ret>;
};
struct function_MaaToolkitFindDeviceWithAdb {
  constexpr static auto func = MaaToolkitFindDeviceWithAdb;
  constexpr static const char* name = "MaaToolkitFindDeviceWithAdb";
  using type = func_type_MaaToolkitFindDeviceWithAdb;
};

struct func_type_MaaToolkitPostFindDevice {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitPostFindDevice {
  constexpr static auto func = MaaToolkitPostFindDevice;
  constexpr static const char* name = "MaaToolkitPostFindDevice";
  using type = func_type_MaaToolkitPostFindDevice;
};

struct func_type_MaaToolkitPostFindDeviceWithAdb {
  struct _0_adb_path {
    constexpr static size_t index = 0;
    constexpr static const char* name = "adb_path";
    using type = const char *;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_adb_path, ret>;
};
struct function_MaaToolkitPostFindDeviceWithAdb {
  constexpr static auto func = MaaToolkitPostFindDeviceWithAdb;
  constexpr static const char* name = "MaaToolkitPostFindDeviceWithAdb";
  using type = func_type_MaaToolkitPostFindDeviceWithAdb;
};

struct func_type_MaaToolkitIsFindDeviceCompleted {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitIsFindDeviceCompleted {
  constexpr static auto func = MaaToolkitIsFindDeviceCompleted;
  constexpr static const char* name = "MaaToolkitIsFindDeviceCompleted";
  using type = func_type_MaaToolkitIsFindDeviceCompleted;
};

struct func_type_MaaToolkitWaitForFindDeviceToComplete {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = unsigned long long;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitWaitForFindDeviceToComplete {
  constexpr static auto func = MaaToolkitWaitForFindDeviceToComplete;
  constexpr static const char* name = "MaaToolkitWaitForFindDeviceToComplete";
  using type = func_type_MaaToolkitWaitForFindDeviceToComplete;
};

struct func_type_MaaToolkitGetDeviceCount {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = unsigned long long;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitGetDeviceCount {
  constexpr static auto func = MaaToolkitGetDeviceCount;
  constexpr static const char* name = "MaaToolkitGetDeviceCount";
  using type = func_type_MaaToolkitGetDeviceCount;
};

struct func_type_MaaToolkitGetDeviceName {
  struct _0_index {
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = unsigned long long;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = const char *;
  };
  using args = std::tuple<_0_index, ret>;
};
struct function_MaaToolkitGetDeviceName {
  constexpr static auto func = MaaToolkitGetDeviceName;
  constexpr static const char* name = "MaaToolkitGetDeviceName";
  using type = func_type_MaaToolkitGetDeviceName;
};

struct func_type_MaaToolkitGetDeviceAdbPath {
  struct _0_index {
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = unsigned long long;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = const char *;
  };
  using args = std::tuple<_0_index, ret>;
};
struct function_MaaToolkitGetDeviceAdbPath {
  constexpr static auto func = MaaToolkitGetDeviceAdbPath;
  constexpr static const char* name = "MaaToolkitGetDeviceAdbPath";
  using type = func_type_MaaToolkitGetDeviceAdbPath;
};

struct func_type_MaaToolkitGetDeviceAdbSerial {
  struct _0_index {
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = unsigned long long;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = const char *;
  };
  using args = std::tuple<_0_index, ret>;
};
struct function_MaaToolkitGetDeviceAdbSerial {
  constexpr static auto func = MaaToolkitGetDeviceAdbSerial;
  constexpr static const char* name = "MaaToolkitGetDeviceAdbSerial";
  using type = func_type_MaaToolkitGetDeviceAdbSerial;
};

struct func_type_MaaToolkitGetDeviceAdbControllerType {
  struct _0_index {
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = unsigned long long;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = int;
  };
  using args = std::tuple<_0_index, ret>;
};
struct function_MaaToolkitGetDeviceAdbControllerType {
  constexpr static auto func = MaaToolkitGetDeviceAdbControllerType;
  constexpr static const char* name = "MaaToolkitGetDeviceAdbControllerType";
  using type = func_type_MaaToolkitGetDeviceAdbControllerType;
};

struct func_type_MaaToolkitGetDeviceAdbConfig {
  struct _0_index {
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = unsigned long long;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = const char *;
  };
  using args = std::tuple<_0_index, ret>;
};
struct function_MaaToolkitGetDeviceAdbConfig {
  constexpr static auto func = MaaToolkitGetDeviceAdbConfig;
  constexpr static const char* name = "MaaToolkitGetDeviceAdbConfig";
  using type = func_type_MaaToolkitGetDeviceAdbConfig;
};

struct func_type_MaaToolkitRegisterCustomRecognizerExecutor {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaInstanceAPI *;
  };
  struct _1_recognizer_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "recognizer_name";
    using type = const char *;
  };
  struct _2_recognizer_exec_path {
    constexpr static size_t index = 2;
    constexpr static const char* name = "recognizer_exec_path";
    using type = const char *;
  };
  struct _3_recognizer_exec_param_json {
    constexpr static size_t index = 3;
    constexpr static const char* name = "recognizer_exec_param_json";
    using type = const char *;
  };
  struct ret {
    constexpr static size_t index = 4;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_handle, _1_recognizer_name, _2_recognizer_exec_path, _3_recognizer_exec_param_json, ret>;
};
struct function_MaaToolkitRegisterCustomRecognizerExecutor {
  constexpr static auto func = MaaToolkitRegisterCustomRecognizerExecutor;
  constexpr static const char* name = "MaaToolkitRegisterCustomRecognizerExecutor";
  using type = func_type_MaaToolkitRegisterCustomRecognizerExecutor;
};

struct func_type_MaaToolkitUnregisterCustomRecognizerExecutor {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaInstanceAPI *;
  };
  struct _1_recognizer_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "recognizer_name";
    using type = const char *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_handle, _1_recognizer_name, ret>;
};
struct function_MaaToolkitUnregisterCustomRecognizerExecutor {
  constexpr static auto func = MaaToolkitUnregisterCustomRecognizerExecutor;
  constexpr static const char* name = "MaaToolkitUnregisterCustomRecognizerExecutor";
  using type = func_type_MaaToolkitUnregisterCustomRecognizerExecutor;
};

struct func_type_MaaToolkitRegisterCustomActionExecutor {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaInstanceAPI *;
  };
  struct _1_action_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "action_name";
    using type = const char *;
  };
  struct _2_action_exec_path {
    constexpr static size_t index = 2;
    constexpr static const char* name = "action_exec_path";
    using type = const char *;
  };
  struct _3_action_exec_param_json {
    constexpr static size_t index = 3;
    constexpr static const char* name = "action_exec_param_json";
    using type = const char *;
  };
  struct ret {
    constexpr static size_t index = 4;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_handle, _1_action_name, _2_action_exec_path, _3_action_exec_param_json, ret>;
};
struct function_MaaToolkitRegisterCustomActionExecutor {
  constexpr static auto func = MaaToolkitRegisterCustomActionExecutor;
  constexpr static const char* name = "MaaToolkitRegisterCustomActionExecutor";
  using type = func_type_MaaToolkitRegisterCustomActionExecutor;
};

struct func_type_MaaToolkitUnregisterCustomActionExecutor {
  struct _0_handle {
    constexpr static size_t index = 0;
    constexpr static const char* name = "handle";
    using type = MaaInstanceAPI *;
  };
  struct _1_action_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "action_name";
    using type = const char *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_handle, _1_action_name, ret>;
};
struct function_MaaToolkitUnregisterCustomActionExecutor {
  constexpr static auto func = MaaToolkitUnregisterCustomActionExecutor;
  constexpr static const char* name = "MaaToolkitUnregisterCustomActionExecutor";
  using type = func_type_MaaToolkitUnregisterCustomActionExecutor;
};

struct func_type_MaaToolkitFindWindow {
  struct _0_class_name {
    constexpr static size_t index = 0;
    constexpr static const char* name = "class_name";
    using type = const char *;
  };
  struct _1_window_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "window_name";
    using type = const char *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = unsigned long long;
  };
  using args = std::tuple<_0_class_name, _1_window_name, ret>;
};
struct function_MaaToolkitFindWindow {
  constexpr static auto func = MaaToolkitFindWindow;
  constexpr static const char* name = "MaaToolkitFindWindow";
  using type = func_type_MaaToolkitFindWindow;
};

struct func_type_MaaToolkitSearchWindow {
  struct _0_class_name {
    constexpr static size_t index = 0;
    constexpr static const char* name = "class_name";
    using type = const char *;
  };
  struct _1_window_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "window_name";
    using type = const char *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = unsigned long long;
  };
  using args = std::tuple<_0_class_name, _1_window_name, ret>;
};
struct function_MaaToolkitSearchWindow {
  constexpr static auto func = MaaToolkitSearchWindow;
  constexpr static const char* name = "MaaToolkitSearchWindow";
  using type = func_type_MaaToolkitSearchWindow;
};

struct func_type_MaaToolkitGetWindow {
  struct _0_index {
    constexpr static size_t index = 0;
    constexpr static const char* name = "index";
    using type = unsigned long long;
  };
  struct ret {
    constexpr static size_t index = 1;
    constexpr static const char* name = "return";
    using type = unsigned long long;
  };
  using args = std::tuple<_0_index, ret>;
};
struct function_MaaToolkitGetWindow {
  constexpr static auto func = MaaToolkitGetWindow;
  constexpr static const char* name = "MaaToolkitGetWindow";
  using type = func_type_MaaToolkitGetWindow;
};

struct func_type_MaaToolkitGetCursorWindow {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = unsigned long long;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitGetCursorWindow {
  constexpr static auto func = MaaToolkitGetCursorWindow;
  constexpr static const char* name = "MaaToolkitGetCursorWindow";
  using type = func_type_MaaToolkitGetCursorWindow;
};

struct func_type_MaaToolkitGetDesktopWindow {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = unsigned long long;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitGetDesktopWindow {
  constexpr static auto func = MaaToolkitGetDesktopWindow;
  constexpr static const char* name = "MaaToolkitGetDesktopWindow";
  using type = func_type_MaaToolkitGetDesktopWindow;
};

struct func_type_MaaToolkitGetForegroundWindow {
  struct ret {
    constexpr static size_t index = 0;
    constexpr static const char* name = "return";
    using type = unsigned long long;
  };
  using args = std::tuple<ret>;
};
struct function_MaaToolkitGetForegroundWindow {
  constexpr static auto func = MaaToolkitGetForegroundWindow;
  constexpr static const char* name = "MaaToolkitGetForegroundWindow";
  using type = func_type_MaaToolkitGetForegroundWindow;
};

struct func_type_MaaControllerSetOptionString {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_key {
    constexpr static size_t index = 1;
    constexpr static const char* name = "key";
    using type = int;
  };
  struct _2_value {
    constexpr static size_t index = 2;
    constexpr static const char* name = "value";
    using type = const char *;
  };
  struct ret {
    constexpr static size_t index = 3;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_ctrl, _1_key, _2_value, ret>;
};
struct function_MaaControllerSetOptionString {
  constexpr static auto func = MaaControllerSetOptionString;
  constexpr static const char* name = "MaaControllerSetOptionString";
  using type = func_type_MaaControllerSetOptionString;
};

struct func_type_MaaControllerSetOptionInteger {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_key {
    constexpr static size_t index = 1;
    constexpr static const char* name = "key";
    using type = int;
  };
  struct _2_value {
    constexpr static size_t index = 2;
    constexpr static const char* name = "value";
    using type = int;
  };
  struct ret {
    constexpr static size_t index = 3;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_ctrl, _1_key, _2_value, ret>;
};
struct function_MaaControllerSetOptionInteger {
  constexpr static auto func = MaaControllerSetOptionInteger;
  constexpr static const char* name = "MaaControllerSetOptionInteger";
  using type = func_type_MaaControllerSetOptionInteger;
};

struct func_type_MaaControllerSetOptionBoolean {
  struct _0_ctrl {
    constexpr static size_t index = 0;
    constexpr static const char* name = "ctrl";
    using type = MaaControllerAPI *;
  };
  struct _1_key {
    constexpr static size_t index = 1;
    constexpr static const char* name = "key";
    using type = int;
  };
  struct _2_value {
    constexpr static size_t index = 2;
    constexpr static const char* name = "value";
    using type = bool;
  };
  struct ret {
    constexpr static size_t index = 3;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_ctrl, _1_key, _2_value, ret>;
};
struct function_MaaControllerSetOptionBoolean {
  constexpr static auto func = MaaControllerSetOptionBoolean;
  constexpr static const char* name = "MaaControllerSetOptionBoolean";
  using type = func_type_MaaControllerSetOptionBoolean;
};

struct func_type_MaaSetGlobalOptionString {
  struct _0_key {
    constexpr static size_t index = 0;
    constexpr static const char* name = "key";
    using type = int;
  };
  struct _1_value {
    constexpr static size_t index = 1;
    constexpr static const char* name = "value";
    using type = const char *;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_key, _1_value, ret>;
};
struct function_MaaSetGlobalOptionString {
  constexpr static auto func = MaaSetGlobalOptionString;
  constexpr static const char* name = "MaaSetGlobalOptionString";
  using type = func_type_MaaSetGlobalOptionString;
};

struct func_type_MaaSetGlobalOptionInteger {
  struct _0_key {
    constexpr static size_t index = 0;
    constexpr static const char* name = "key";
    using type = int;
  };
  struct _1_value {
    constexpr static size_t index = 1;
    constexpr static const char* name = "value";
    using type = int;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_key, _1_value, ret>;
};
struct function_MaaSetGlobalOptionInteger {
  constexpr static auto func = MaaSetGlobalOptionInteger;
  constexpr static const char* name = "MaaSetGlobalOptionInteger";
  using type = func_type_MaaSetGlobalOptionInteger;
};

struct func_type_MaaSetGlobalOptionBoolean {
  struct _0_key {
    constexpr static size_t index = 0;
    constexpr static const char* name = "key";
    using type = int;
  };
  struct _1_value {
    constexpr static size_t index = 1;
    constexpr static const char* name = "value";
    using type = bool;
  };
  struct ret {
    constexpr static size_t index = 2;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_key, _1_value, ret>;
};
struct function_MaaSetGlobalOptionBoolean {
  constexpr static auto func = MaaSetGlobalOptionBoolean;
  constexpr static const char* name = "MaaSetGlobalOptionBoolean";
  using type = func_type_MaaSetGlobalOptionBoolean;
};

struct func_type_MaaRegisterCustomRecognizerImpl {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "name";
    using type = const char *;
  };
  struct _2_analyze {
    constexpr static size_t index = 2;
    constexpr static const char* name = "analyze";
    using type = unsigned char (*)(MaaSyncContextAPI *, MaaImageBuffer *, const char *, const char *, void *, MaaRect *, MaaStringBuffer *);
  };
  struct _3_analyze_arg {
    constexpr static size_t index = 3;
    constexpr static const char* name = "analyze_arg";
    using type = void *;
  };
  struct ret {
    constexpr static size_t index = 4;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _1_name, _2_analyze, _3_analyze_arg, ret>;
};
struct function_MaaRegisterCustomRecognizerImpl {
  constexpr static auto func = MaaRegisterCustomRecognizerImpl;
  constexpr static const char* name = "MaaRegisterCustomRecognizerImpl";
  using type = func_type_MaaRegisterCustomRecognizerImpl;
};

struct func_type_MaaRegisterCustomActionImpl {
  struct _0_inst {
    constexpr static size_t index = 0;
    constexpr static const char* name = "inst";
    using type = MaaInstanceAPI *;
  };
  struct _1_name {
    constexpr static size_t index = 1;
    constexpr static const char* name = "name";
    using type = const char *;
  };
  struct _2_run {
    constexpr static size_t index = 2;
    constexpr static const char* name = "run";
    using type = unsigned char (*)(MaaSyncContextAPI *, const char *, const char *, MaaRect *, const char *, void *);
  };
  struct _3_run_arg {
    constexpr static size_t index = 3;
    constexpr static const char* name = "run_arg";
    using type = void *;
  };
  struct _4_stop {
    constexpr static size_t index = 4;
    constexpr static const char* name = "stop";
    using type = void (*)(void *);
  };
  struct _5_stop_arg {
    constexpr static size_t index = 5;
    constexpr static const char* name = "stop_arg";
    using type = void *;
  };
  struct ret {
    constexpr static size_t index = 6;
    constexpr static const char* name = "return";
    using type = unsigned char;
  };
  using args = std::tuple<_0_inst, _1_name, _2_run, _3_run_arg, _4_stop, _5_stop_arg, ret>;
};
struct function_MaaRegisterCustomActionImpl {
  constexpr static auto func = MaaRegisterCustomActionImpl;
  constexpr static const char* name = "MaaRegisterCustomActionImpl";
  using type = func_type_MaaRegisterCustomActionImpl;
};

using __function_list = std::tuple<
  function_MaaAdbControllerCreate,
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
  function_MaaUnregisterCustomRecognizer,
  function_MaaClearCustomRecognizer,
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
  function_MaaSetGlobalOptionBoolean,
  function_MaaRegisterCustomRecognizerImpl,
  function_MaaRegisterCustomActionImpl
>;

}

template<>
struct is_input<maa::func_type_MaaAdbControllerCreate::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaAdbControllerCreate::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaAdbControllerCreate::ret::type;
};

template<>
struct is_input<maa::func_type_MaaAdbControllerCreateV2::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaAdbControllerCreateV2::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaAdbControllerCreateV2::ret::type;
};

template<>
struct is_input<maa::func_type_MaaThriftControllerCreate::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaThriftControllerCreate::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaThriftControllerCreate::ret::type;
};

template<>
struct is_input<maa::func_type_MaaDbgControllerCreate::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaDbgControllerCreate::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaDbgControllerCreate::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerDestroy::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerDestroy::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerDestroy::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerPostConnection::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerPostConnection::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerPostConnection::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerPostClick::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerPostClick::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerPostClick::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerPostSwipe::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerPostSwipe::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerPostSwipe::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerPostPressKey::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerPostPressKey::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerPostPressKey::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerPostInputText::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerPostInputText::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerPostInputText::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerPostTouchDown::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerPostTouchDown::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerPostTouchDown::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerPostTouchMove::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerPostTouchMove::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerPostTouchMove::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerPostTouchUp::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerPostTouchUp::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerPostTouchUp::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerPostScreencap::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerPostScreencap::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerPostScreencap::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerStatus::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerStatus::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerStatus::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerWait::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerWait::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerWait::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerConnected::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerConnected::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerConnected::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerGetImage::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerGetImage::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerGetImage::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerGetUUID::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerGetUUID::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerGetUUID::ret::type;
};

template<>
struct is_input<maa::func_type_MaaCreate::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaCreate::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaCreate::ret::type;
};

template<>
struct is_input<maa::func_type_MaaDestroy::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaDestroy::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaDestroy::ret::type;
};

template<>
struct is_input<maa::func_type_MaaBindResource::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaBindResource::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaBindResource::ret::type;
};

template<>
struct is_input<maa::func_type_MaaBindController::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaBindController::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaBindController::ret::type;
};

template<>
struct is_input<maa::func_type_MaaInited::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaInited::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaInited::ret::type;
};

template<>
struct is_input<maa::func_type_MaaUnregisterCustomRecognizer::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaUnregisterCustomRecognizer::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaUnregisterCustomRecognizer::ret::type;
};

template<>
struct is_input<maa::func_type_MaaClearCustomRecognizer::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaClearCustomRecognizer::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaClearCustomRecognizer::ret::type;
};

template<>
struct is_input<maa::func_type_MaaUnregisterCustomAction::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaUnregisterCustomAction::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaUnregisterCustomAction::ret::type;
};

template<>
struct is_input<maa::func_type_MaaClearCustomAction::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaClearCustomAction::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaClearCustomAction::ret::type;
};

template<>
struct is_input<maa::func_type_MaaPostTask::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaPostTask::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaPostTask::ret::type;
};

template<>
struct is_input<maa::func_type_MaaSetTaskParam::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaSetTaskParam::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaSetTaskParam::ret::type;
};

template<>
struct is_input<maa::func_type_MaaTaskStatus::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaTaskStatus::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaTaskStatus::ret::type;
};

template<>
struct is_input<maa::func_type_MaaWaitTask::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaWaitTask::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaWaitTask::ret::type;
};

template<>
struct is_input<maa::func_type_MaaTaskAllFinished::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaTaskAllFinished::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaTaskAllFinished::ret::type;
};

template<>
struct is_input<maa::func_type_MaaPostStop::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaPostStop::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaPostStop::ret::type;
};

template<>
struct is_input<maa::func_type_MaaStop::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaStop::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaStop::ret::type;
};

template<>
struct is_input<maa::func_type_MaaGetResource::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaGetResource::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaGetResource::ret::type;
};

template<>
struct is_input<maa::func_type_MaaGetController::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaGetController::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaGetController::ret::type;
};

template<>
struct is_input<maa::func_type_MaaResourceCreate::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaResourceCreate::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaResourceCreate::ret::type;
};

template<>
struct is_input<maa::func_type_MaaResourceDestroy::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaResourceDestroy::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaResourceDestroy::ret::type;
};

template<>
struct is_input<maa::func_type_MaaResourcePostPath::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaResourcePostPath::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaResourcePostPath::ret::type;
};

template<>
struct is_input<maa::func_type_MaaResourceStatus::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaResourceStatus::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaResourceStatus::ret::type;
};

template<>
struct is_input<maa::func_type_MaaResourceWait::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaResourceWait::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaResourceWait::ret::type;
};

template<>
struct is_input<maa::func_type_MaaResourceLoaded::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaResourceLoaded::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaResourceLoaded::ret::type;
};

template<>
struct is_input<maa::func_type_MaaResourceGetHash::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaResourceGetHash::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaResourceGetHash::ret::type;
};

template<>
struct is_input<maa::func_type_MaaResourceGetTaskList::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaResourceGetTaskList::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaResourceGetTaskList::ret::type;
};

template<>
struct is_input<maa::func_type_MaaCreateImageBuffer::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaCreateImageBuffer::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaCreateImageBuffer::ret::type;
};

template<>
struct is_input<maa::func_type_MaaDestroyImageBuffer::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaDestroyImageBuffer::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaDestroyImageBuffer::ret::type;
};

template<>
struct is_input<maa::func_type_MaaIsImageEmpty::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaIsImageEmpty::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaIsImageEmpty::ret::type;
};

template<>
struct is_input<maa::func_type_MaaClearImage::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaClearImage::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaClearImage::ret::type;
};

template<>
struct is_input<maa::func_type_MaaGetImageWidth::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaGetImageWidth::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaGetImageWidth::ret::type;
};

template<>
struct is_input<maa::func_type_MaaGetImageHeight::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaGetImageHeight::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaGetImageHeight::ret::type;
};

template<>
struct is_input<maa::func_type_MaaGetImageType::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaGetImageType::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaGetImageType::ret::type;
};

template<>
struct is_input<maa::func_type_MaaGetImageEncoded::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaGetImageEncoded::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaGetImageEncoded::ret::type;
};

template<>
struct is_input<maa::func_type_MaaSetImageEncoded::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaSetImageEncoded::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaSetImageEncoded::ret::type;
};

template<>
struct is_input<maa::func_type_MaaVersion::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaVersion::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaVersion::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitInit::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitInit::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitInit::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitUninit::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitUninit::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitUninit::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitFindDevice::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitFindDevice::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitFindDevice::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitFindDeviceWithAdb::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitFindDeviceWithAdb::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitFindDeviceWithAdb::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitPostFindDevice::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitPostFindDevice::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitPostFindDevice::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitPostFindDeviceWithAdb::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitPostFindDeviceWithAdb::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitPostFindDeviceWithAdb::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitIsFindDeviceCompleted::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitIsFindDeviceCompleted::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitIsFindDeviceCompleted::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitWaitForFindDeviceToComplete::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitWaitForFindDeviceToComplete::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitWaitForFindDeviceToComplete::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitGetDeviceCount::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetDeviceCount::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitGetDeviceCount::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitGetDeviceName::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetDeviceName::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitGetDeviceName::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitGetDeviceAdbPath::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetDeviceAdbPath::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitGetDeviceAdbPath::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitGetDeviceAdbSerial::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetDeviceAdbSerial::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitGetDeviceAdbSerial::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitGetDeviceAdbControllerType::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetDeviceAdbControllerType::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitGetDeviceAdbControllerType::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitGetDeviceAdbConfig::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetDeviceAdbConfig::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitGetDeviceAdbConfig::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitRegisterCustomRecognizerExecutor::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitRegisterCustomRecognizerExecutor::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitRegisterCustomRecognizerExecutor::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitUnregisterCustomRecognizerExecutor::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitUnregisterCustomRecognizerExecutor::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitUnregisterCustomRecognizerExecutor::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitRegisterCustomActionExecutor::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitRegisterCustomActionExecutor::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitRegisterCustomActionExecutor::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitUnregisterCustomActionExecutor::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitUnregisterCustomActionExecutor::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitUnregisterCustomActionExecutor::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitFindWindow::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitFindWindow::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitFindWindow::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitSearchWindow::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitSearchWindow::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitSearchWindow::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitGetWindow::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetWindow::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitGetWindow::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitGetCursorWindow::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetCursorWindow::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitGetCursorWindow::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitGetDesktopWindow::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetDesktopWindow::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitGetDesktopWindow::ret::type;
};

template<>
struct is_input<maa::func_type_MaaToolkitGetForegroundWindow::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaToolkitGetForegroundWindow::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaToolkitGetForegroundWindow::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerSetOptionString::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerSetOptionString::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerSetOptionString::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerSetOptionInteger::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerSetOptionInteger::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerSetOptionInteger::ret::type;
};

template<>
struct is_input<maa::func_type_MaaControllerSetOptionBoolean::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaControllerSetOptionBoolean::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaControllerSetOptionBoolean::ret::type;
};

template<>
struct is_input<maa::func_type_MaaSetGlobalOptionString::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaSetGlobalOptionString::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaSetGlobalOptionString::ret::type;
};

template<>
struct is_input<maa::func_type_MaaSetGlobalOptionInteger::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaSetGlobalOptionInteger::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaSetGlobalOptionInteger::ret::type;
};

template<>
struct is_input<maa::func_type_MaaSetGlobalOptionBoolean::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaSetGlobalOptionBoolean::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaSetGlobalOptionBoolean::ret::type;
};

template<>
struct is_input<maa::func_type_MaaRegisterCustomRecognizerImpl::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaRegisterCustomRecognizerImpl::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaRegisterCustomRecognizerImpl::ret::type;
};

template<>
struct is_input<maa::func_type_MaaRegisterCustomActionImpl::ret, false> {
  constexpr static bool value = false;
  using type = std::monostate;
};
template<>
struct is_output<maa::func_type_MaaRegisterCustomActionImpl::ret, false> {
  constexpr static bool value = true;
  using type = maa::func_type_MaaRegisterCustomActionImpl::ret::type;
};


}