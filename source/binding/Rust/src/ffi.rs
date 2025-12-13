use crate::define::*;
use crate::{framework, toolkit};
use libloading::Symbol;
use std::ffi::{c_char, c_int, c_void};

macro_rules! load_symbol {
    ($lib:expr, $name:ident, $ty:ty) => {{
        let sym: Symbol<$ty> = unsafe { $lib.get(stringify!($name).as_bytes()) }
            .expect(concat!("Failed to load symbol: ", stringify!($name)));
        *sym
    }};
}

// MaaUtility.h
pub fn maa_version() -> *const c_char {
    type Fn = extern "C" fn() -> *const c_char;
    load_symbol!(framework(), MaaVersion, Fn)()
}

pub fn maa_global_set_option(
    key: MaaGlobalOption,
    value: MaaOptionValue,
    val_size: MaaOptionValueSize,
) -> MaaBool {
    type Fn = extern "C" fn(MaaGlobalOption, MaaOptionValue, MaaOptionValueSize) -> MaaBool;
    load_symbol!(framework(), MaaGlobalSetOption, Fn)(key, value, val_size)
}

pub fn maa_global_load_plugin(path: *const c_char) -> MaaBool {
    type Fn = extern "C" fn(*const c_char) -> MaaBool;
    load_symbol!(framework(), MaaGlobalLoadPlugin, Fn)(path)
}

// MaaBuffer.h
pub fn maa_string_buffer_create() -> MaaStringBufferHandle {
    type Fn = extern "C" fn() -> MaaStringBufferHandle;
    load_symbol!(framework(), MaaStringBufferCreate, Fn)()
}

pub fn maa_string_buffer_destroy(handle: MaaStringBufferHandle) {
    type Fn = extern "C" fn(MaaStringBufferHandle);
    load_symbol!(framework(), MaaStringBufferDestroy, Fn)(handle)
}

pub fn maa_string_buffer_get(handle: MaaStringBufferHandle) -> *const c_char {
    type Fn = extern "C" fn(MaaStringBufferHandle) -> *const c_char;
    load_symbol!(framework(), MaaStringBufferGet, Fn)(handle)
}

pub fn maa_string_buffer_size(handle: MaaStringBufferHandle) -> MaaSize {
    type Fn = extern "C" fn(MaaStringBufferHandle) -> MaaSize;
    load_symbol!(framework(), MaaStringBufferSize, Fn)(handle)
}

pub fn maa_string_buffer_set(handle: MaaStringBufferHandle, str: *const c_char) -> MaaBool {
    type Fn = extern "C" fn(MaaStringBufferHandle, *const c_char) -> MaaBool;
    load_symbol!(framework(), MaaStringBufferSet, Fn)(handle, str)
}

pub fn maa_image_buffer_create() -> MaaImageBufferHandle {
    type Fn = extern "C" fn() -> MaaImageBufferHandle;
    load_symbol!(framework(), MaaImageBufferCreate, Fn)()
}

pub fn maa_image_buffer_destroy(handle: MaaImageBufferHandle) {
    type Fn = extern "C" fn(MaaImageBufferHandle);
    load_symbol!(framework(), MaaImageBufferDestroy, Fn)(handle)
}

pub fn maa_image_buffer_get_raw_data(handle: MaaImageBufferHandle) -> *mut u8 {
    type Fn = extern "C" fn(MaaImageBufferHandle) -> *mut u8;
    load_symbol!(framework(), MaaImageBufferGetRawData, Fn)(handle)
}

pub fn maa_image_buffer_width(handle: MaaImageBufferHandle) -> i32 {
    type Fn = extern "C" fn(MaaImageBufferHandle) -> i32;
    load_symbol!(framework(), MaaImageBufferWidth, Fn)(handle)
}

pub fn maa_image_buffer_height(handle: MaaImageBufferHandle) -> i32 {
    type Fn = extern "C" fn(MaaImageBufferHandle) -> i32;
    load_symbol!(framework(), MaaImageBufferHeight, Fn)(handle)
}

pub fn maa_image_buffer_channels(handle: MaaImageBufferHandle) -> i32 {
    type Fn = extern "C" fn(MaaImageBufferHandle) -> i32;
    load_symbol!(framework(), MaaImageBufferChannels, Fn)(handle)
}

pub fn maa_image_buffer_type(handle: MaaImageBufferHandle) -> i32 {
    type Fn = extern "C" fn(MaaImageBufferHandle) -> i32;
    load_symbol!(framework(), MaaImageBufferType, Fn)(handle)
}

pub fn maa_image_buffer_set_raw_data(
    handle: MaaImageBufferHandle,
    data: *const u8,
    width: i32,
    height: i32,
    img_type: i32,
) -> MaaBool {
    type Fn = extern "C" fn(MaaImageBufferHandle, *const u8, i32, i32, i32) -> MaaBool;
    load_symbol!(framework(), MaaImageBufferSetRawData, Fn)(handle, data, width, height, img_type)
}

pub fn maa_rect_create() -> MaaRectHandle {
    type Fn = extern "C" fn() -> MaaRectHandle;
    load_symbol!(framework(), MaaRectCreate, Fn)()
}

pub fn maa_rect_destroy(handle: MaaRectHandle) {
    type Fn = extern "C" fn(MaaRectHandle);
    load_symbol!(framework(), MaaRectDestroy, Fn)(handle)
}

pub fn maa_rect_get_x(handle: MaaRectHandle) -> i32 {
    type Fn = extern "C" fn(MaaRectHandle) -> i32;
    load_symbol!(framework(), MaaRectGetX, Fn)(handle)
}

pub fn maa_rect_get_y(handle: MaaRectHandle) -> i32 {
    type Fn = extern "C" fn(MaaRectHandle) -> i32;
    load_symbol!(framework(), MaaRectGetY, Fn)(handle)
}

pub fn maa_rect_get_w(handle: MaaRectHandle) -> i32 {
    type Fn = extern "C" fn(MaaRectHandle) -> i32;
    load_symbol!(framework(), MaaRectGetW, Fn)(handle)
}

pub fn maa_rect_get_h(handle: MaaRectHandle) -> i32 {
    type Fn = extern "C" fn(MaaRectHandle) -> i32;
    load_symbol!(framework(), MaaRectGetH, Fn)(handle)
}

pub fn maa_rect_set(handle: MaaRectHandle, x: i32, y: i32, w: i32, h: i32) -> MaaBool {
    type Fn = extern "C" fn(MaaRectHandle, i32, i32, i32, i32) -> MaaBool;
    load_symbol!(framework(), MaaRectSet, Fn)(handle, x, y, w, h)
}

pub fn maa_string_list_buffer_create() -> MaaStringListBufferHandle {
    type Fn = extern "C" fn() -> MaaStringListBufferHandle;
    load_symbol!(framework(), MaaStringListBufferCreate, Fn)()
}

pub fn maa_string_list_buffer_destroy(handle: MaaStringListBufferHandle) {
    type Fn = extern "C" fn(MaaStringListBufferHandle);
    load_symbol!(framework(), MaaStringListBufferDestroy, Fn)(handle)
}

pub fn maa_string_list_buffer_size(handle: MaaStringListBufferHandle) -> MaaSize {
    type Fn = extern "C" fn(MaaStringListBufferHandle) -> MaaSize;
    load_symbol!(framework(), MaaStringListBufferSize, Fn)(handle)
}

pub fn maa_string_list_buffer_at(
    handle: MaaStringListBufferHandle,
    index: MaaSize,
) -> MaaStringBufferHandle {
    type Fn = extern "C" fn(MaaStringListBufferHandle, MaaSize) -> MaaStringBufferHandle;
    load_symbol!(framework(), MaaStringListBufferAt, Fn)(handle, index)
}

pub fn maa_string_list_buffer_append(
    handle: MaaStringListBufferHandle,
    value: MaaStringBufferHandle,
) -> MaaBool {
    type Fn = extern "C" fn(MaaStringListBufferHandle, MaaStringBufferHandle) -> MaaBool;
    load_symbol!(framework(), MaaStringListBufferAppend, Fn)(handle, value)
}

pub fn maa_image_list_buffer_create() -> MaaImageListBufferHandle {
    type Fn = extern "C" fn() -> MaaImageListBufferHandle;
    load_symbol!(framework(), MaaImageListBufferCreate, Fn)()
}

pub fn maa_image_list_buffer_destroy(handle: MaaImageListBufferHandle) {
    type Fn = extern "C" fn(MaaImageListBufferHandle);
    load_symbol!(framework(), MaaImageListBufferDestroy, Fn)(handle)
}

pub fn maa_image_list_buffer_size(handle: MaaImageListBufferHandle) -> MaaSize {
    type Fn = extern "C" fn(MaaImageListBufferHandle) -> MaaSize;
    load_symbol!(framework(), MaaImageListBufferSize, Fn)(handle)
}

pub fn maa_image_list_buffer_at(
    handle: MaaImageListBufferHandle,
    index: MaaSize,
) -> MaaImageBufferHandle {
    type Fn = extern "C" fn(MaaImageListBufferHandle, MaaSize) -> MaaImageBufferHandle;
    load_symbol!(framework(), MaaImageListBufferAt, Fn)(handle, index)
}

// MaaResource.h
pub fn maa_resource_create() -> MaaResourceHandle {
    type Fn = extern "C" fn() -> MaaResourceHandle;
    load_symbol!(framework(), MaaResourceCreate, Fn)()
}

pub fn maa_resource_destroy(res: MaaResourceHandle) {
    type Fn = extern "C" fn(MaaResourceHandle);
    load_symbol!(framework(), MaaResourceDestroy, Fn)(res)
}

pub fn maa_resource_add_sink(
    res: MaaResourceHandle,
    callback: MaaEventCallback,
    trans_arg: *mut c_void,
) -> MaaSinkId {
    type Fn =
        extern "C" fn(MaaResourceHandle, MaaEventCallback, *mut c_void) -> MaaSinkId;
    load_symbol!(framework(), MaaResourceAddSink, Fn)(res, callback, trans_arg)
}

pub fn maa_resource_remove_sink(res: MaaResourceHandle, sink_id: MaaSinkId) {
    type Fn = extern "C" fn(MaaResourceHandle, MaaSinkId);
    load_symbol!(framework(), MaaResourceRemoveSink, Fn)(res, sink_id)
}

pub fn maa_resource_clear_sinks(res: MaaResourceHandle) {
    type Fn = extern "C" fn(MaaResourceHandle);
    load_symbol!(framework(), MaaResourceClearSinks, Fn)(res)
}

pub fn maa_resource_register_custom_recognition(
    res: MaaResourceHandle,
    name: *const c_char,
    recognition: MaaCustomRecognitionCallback,
    trans_arg: *mut c_void,
) -> MaaBool {
    type Fn = extern "C" fn(
        MaaResourceHandle,
        *const c_char,
        MaaCustomRecognitionCallback,
        *mut c_void,
    ) -> MaaBool;
    load_symbol!(framework(), MaaResourceRegisterCustomRecognition, Fn)(
        res, name, recognition, trans_arg,
    )
}

pub fn maa_resource_unregister_custom_recognition(
    res: MaaResourceHandle,
    name: *const c_char,
) -> MaaBool {
    type Fn = extern "C" fn(MaaResourceHandle, *const c_char) -> MaaBool;
    load_symbol!(framework(), MaaResourceUnregisterCustomRecognition, Fn)(res, name)
}

pub fn maa_resource_clear_custom_recognition(res: MaaResourceHandle) -> MaaBool {
    type Fn = extern "C" fn(MaaResourceHandle) -> MaaBool;
    load_symbol!(framework(), MaaResourceClearCustomRecognition, Fn)(res)
}

pub fn maa_resource_register_custom_action(
    res: MaaResourceHandle,
    name: *const c_char,
    action: MaaCustomActionCallback,
    trans_arg: *mut c_void,
) -> MaaBool {
    type Fn = extern "C" fn(
        MaaResourceHandle,
        *const c_char,
        MaaCustomActionCallback,
        *mut c_void,
    ) -> MaaBool;
    load_symbol!(framework(), MaaResourceRegisterCustomAction, Fn)(res, name, action, trans_arg)
}

pub fn maa_resource_unregister_custom_action(
    res: MaaResourceHandle,
    name: *const c_char,
) -> MaaBool {
    type Fn = extern "C" fn(MaaResourceHandle, *const c_char) -> MaaBool;
    load_symbol!(framework(), MaaResourceUnregisterCustomAction, Fn)(res, name)
}

pub fn maa_resource_clear_custom_action(res: MaaResourceHandle) -> MaaBool {
    type Fn = extern "C" fn(MaaResourceHandle) -> MaaBool;
    load_symbol!(framework(), MaaResourceClearCustomAction, Fn)(res)
}

pub fn maa_resource_post_bundle(res: MaaResourceHandle, path: *const c_char) -> MaaResId {
    type Fn = extern "C" fn(MaaResourceHandle, *const c_char) -> MaaResId;
    load_symbol!(framework(), MaaResourcePostBundle, Fn)(res, path)
}

pub fn maa_resource_override_pipeline(
    res: MaaResourceHandle,
    pipeline_override: *const c_char,
) -> MaaBool {
    type Fn = extern "C" fn(MaaResourceHandle, *const c_char) -> MaaBool;
    load_symbol!(framework(), MaaResourceOverridePipeline, Fn)(res, pipeline_override)
}

pub fn maa_resource_override_next(
    res: MaaResourceHandle,
    node_name: *const c_char,
    next_list: MaaStringListBufferHandle,
) -> MaaBool {
    type Fn = extern "C" fn(MaaResourceHandle, *const c_char, MaaStringListBufferHandle) -> MaaBool;
    load_symbol!(framework(), MaaResourceOverrideNext, Fn)(res, node_name, next_list)
}

pub fn maa_resource_override_image(
    res: MaaResourceHandle,
    image_name: *const c_char,
    image: MaaImageBufferHandle,
) -> MaaBool {
    type Fn = extern "C" fn(MaaResourceHandle, *const c_char, MaaImageBufferHandle) -> MaaBool;
    load_symbol!(framework(), MaaResourceOverrideImage, Fn)(res, image_name, image)
}

pub fn maa_resource_get_node_data(
    res: MaaResourceHandle,
    node_name: *const c_char,
    buffer: MaaStringBufferHandle,
) -> MaaBool {
    type Fn = extern "C" fn(MaaResourceHandle, *const c_char, MaaStringBufferHandle) -> MaaBool;
    load_symbol!(framework(), MaaResourceGetNodeData, Fn)(res, node_name, buffer)
}

pub fn maa_resource_clear(res: MaaResourceHandle) -> MaaBool {
    type Fn = extern "C" fn(MaaResourceHandle) -> MaaBool;
    load_symbol!(framework(), MaaResourceClear, Fn)(res)
}

pub fn maa_resource_status(res: MaaResourceHandle, id: MaaResId) -> MaaStatus {
    type Fn = extern "C" fn(MaaResourceHandle, MaaResId) -> MaaStatus;
    load_symbol!(framework(), MaaResourceStatus, Fn)(res, id)
}

pub fn maa_resource_wait(res: MaaResourceHandle, id: MaaResId) -> MaaStatus {
    type Fn = extern "C" fn(MaaResourceHandle, MaaResId) -> MaaStatus;
    load_symbol!(framework(), MaaResourceWait, Fn)(res, id)
}

pub fn maa_resource_loaded(res: MaaResourceHandle) -> MaaBool {
    type Fn = extern "C" fn(MaaResourceHandle) -> MaaBool;
    load_symbol!(framework(), MaaResourceLoaded, Fn)(res)
}

pub fn maa_resource_set_option(
    res: MaaResourceHandle,
    key: MaaResOption,
    value: MaaOptionValue,
    val_size: MaaOptionValueSize,
) -> MaaBool {
    type Fn =
        extern "C" fn(MaaResourceHandle, MaaResOption, MaaOptionValue, MaaOptionValueSize) -> MaaBool;
    load_symbol!(framework(), MaaResourceSetOption, Fn)(res, key, value, val_size)
}

pub fn maa_resource_get_hash(res: MaaResourceHandle, buffer: MaaStringBufferHandle) -> MaaBool {
    type Fn = extern "C" fn(MaaResourceHandle, MaaStringBufferHandle) -> MaaBool;
    load_symbol!(framework(), MaaResourceGetHash, Fn)(res, buffer)
}

pub fn maa_resource_get_node_list(
    res: MaaResourceHandle,
    buffer: MaaStringListBufferHandle,
) -> MaaBool {
    type Fn = extern "C" fn(MaaResourceHandle, MaaStringListBufferHandle) -> MaaBool;
    load_symbol!(framework(), MaaResourceGetNodeList, Fn)(res, buffer)
}

pub fn maa_resource_get_custom_recognition_list(
    res: MaaResourceHandle,
    buffer: MaaStringListBufferHandle,
) -> MaaBool {
    type Fn = extern "C" fn(MaaResourceHandle, MaaStringListBufferHandle) -> MaaBool;
    load_symbol!(framework(), MaaResourceGetCustomRecognitionList, Fn)(res, buffer)
}

pub fn maa_resource_get_custom_action_list(
    res: MaaResourceHandle,
    buffer: MaaStringListBufferHandle,
) -> MaaBool {
    type Fn = extern "C" fn(MaaResourceHandle, MaaStringListBufferHandle) -> MaaBool;
    load_symbol!(framework(), MaaResourceGetCustomActionList, Fn)(res, buffer)
}

// MaaController.h
pub fn maa_adb_controller_create(
    adb_path: *const c_char,
    address: *const c_char,
    screencap_methods: MaaAdbScreencapMethod,
    input_methods: MaaAdbInputMethod,
    config: *const c_char,
    agent_path: *const c_char,
) -> MaaControllerHandle {
    type Fn = extern "C" fn(
        *const c_char,
        *const c_char,
        MaaAdbScreencapMethod,
        MaaAdbInputMethod,
        *const c_char,
        *const c_char,
    ) -> MaaControllerHandle;
    load_symbol!(framework(), MaaAdbControllerCreate, Fn)(
        adb_path,
        address,
        screencap_methods,
        input_methods,
        config,
        agent_path,
    )
}

pub fn maa_win32_controller_create(
    hwnd: *mut c_void,
    screencap_method: MaaWin32ScreencapMethod,
    mouse_method: MaaWin32InputMethod,
    keyboard_method: MaaWin32InputMethod,
) -> MaaControllerHandle {
    type Fn = extern "C" fn(
        *mut c_void,
        MaaWin32ScreencapMethod,
        MaaWin32InputMethod,
        MaaWin32InputMethod,
    ) -> MaaControllerHandle;
    load_symbol!(framework(), MaaWin32ControllerCreate, Fn)(
        hwnd,
        screencap_method,
        mouse_method,
        keyboard_method,
    )
}

pub fn maa_dbg_controller_create(
    read_path: *const c_char,
    write_path: *const c_char,
    dbg_type: MaaDbgControllerType,
    config: *const c_char,
) -> MaaControllerHandle {
    type Fn = extern "C" fn(
        *const c_char,
        *const c_char,
        MaaDbgControllerType,
        *const c_char,
    ) -> MaaControllerHandle;
    load_symbol!(framework(), MaaDbgControllerCreate, Fn)(read_path, write_path, dbg_type, config)
}

pub fn maa_controller_destroy(ctrl: MaaControllerHandle) {
    type Fn = extern "C" fn(MaaControllerHandle);
    load_symbol!(framework(), MaaControllerDestroy, Fn)(ctrl)
}

pub fn maa_controller_add_sink(
    ctrl: MaaControllerHandle,
    callback: MaaEventCallback,
    trans_arg: *mut c_void,
) -> MaaSinkId {
    type Fn =
        extern "C" fn(MaaControllerHandle, MaaEventCallback, *mut c_void) -> MaaSinkId;
    load_symbol!(framework(), MaaControllerAddSink, Fn)(ctrl, callback, trans_arg)
}

pub fn maa_controller_remove_sink(ctrl: MaaControllerHandle, sink_id: MaaSinkId) {
    type Fn = extern "C" fn(MaaControllerHandle, MaaSinkId);
    load_symbol!(framework(), MaaControllerRemoveSink, Fn)(ctrl, sink_id)
}

pub fn maa_controller_clear_sinks(ctrl: MaaControllerHandle) {
    type Fn = extern "C" fn(MaaControllerHandle);
    load_symbol!(framework(), MaaControllerClearSinks, Fn)(ctrl)
}

pub fn maa_controller_set_option(
    ctrl: MaaControllerHandle,
    key: MaaCtrlOption,
    value: MaaOptionValue,
    val_size: MaaOptionValueSize,
) -> MaaBool {
    type Fn = extern "C" fn(
        MaaControllerHandle,
        MaaCtrlOption,
        MaaOptionValue,
        MaaOptionValueSize,
    ) -> MaaBool;
    load_symbol!(framework(), MaaControllerSetOption, Fn)(ctrl, key, value, val_size)
}

pub fn maa_controller_post_connection(ctrl: MaaControllerHandle) -> MaaCtrlId {
    type Fn = extern "C" fn(MaaControllerHandle) -> MaaCtrlId;
    load_symbol!(framework(), MaaControllerPostConnection, Fn)(ctrl)
}

pub fn maa_controller_post_click(ctrl: MaaControllerHandle, x: c_int, y: c_int) -> MaaCtrlId {
    type Fn = extern "C" fn(MaaControllerHandle, c_int, c_int) -> MaaCtrlId;
    load_symbol!(framework(), MaaControllerPostClick, Fn)(ctrl, x, y)
}

pub fn maa_controller_post_swipe(
    ctrl: MaaControllerHandle,
    x1: c_int,
    y1: c_int,
    x2: c_int,
    y2: c_int,
    duration: c_int,
) -> MaaCtrlId {
    type Fn = extern "C" fn(MaaControllerHandle, c_int, c_int, c_int, c_int, c_int) -> MaaCtrlId;
    load_symbol!(framework(), MaaControllerPostSwipe, Fn)(ctrl, x1, y1, x2, y2, duration)
}

pub fn maa_controller_post_click_key(ctrl: MaaControllerHandle, key: c_int) -> MaaCtrlId {
    type Fn = extern "C" fn(MaaControllerHandle, c_int) -> MaaCtrlId;
    load_symbol!(framework(), MaaControllerPostClickKey, Fn)(ctrl, key)
}

pub fn maa_controller_post_key_down(ctrl: MaaControllerHandle, key: c_int) -> MaaCtrlId {
    type Fn = extern "C" fn(MaaControllerHandle, c_int) -> MaaCtrlId;
    load_symbol!(framework(), MaaControllerPostKeyDown, Fn)(ctrl, key)
}

pub fn maa_controller_post_key_up(ctrl: MaaControllerHandle, key: c_int) -> MaaCtrlId {
    type Fn = extern "C" fn(MaaControllerHandle, c_int) -> MaaCtrlId;
    load_symbol!(framework(), MaaControllerPostKeyUp, Fn)(ctrl, key)
}

pub fn maa_controller_post_input_text(
    ctrl: MaaControllerHandle,
    text: *const c_char,
) -> MaaCtrlId {
    type Fn = extern "C" fn(MaaControllerHandle, *const c_char) -> MaaCtrlId;
    load_symbol!(framework(), MaaControllerPostInputText, Fn)(ctrl, text)
}

pub fn maa_controller_post_start_app(
    ctrl: MaaControllerHandle,
    intent: *const c_char,
) -> MaaCtrlId {
    type Fn = extern "C" fn(MaaControllerHandle, *const c_char) -> MaaCtrlId;
    load_symbol!(framework(), MaaControllerPostStartApp, Fn)(ctrl, intent)
}

pub fn maa_controller_post_stop_app(
    ctrl: MaaControllerHandle,
    intent: *const c_char,
) -> MaaCtrlId {
    type Fn = extern "C" fn(MaaControllerHandle, *const c_char) -> MaaCtrlId;
    load_symbol!(framework(), MaaControllerPostStopApp, Fn)(ctrl, intent)
}

pub fn maa_controller_post_touch_down(
    ctrl: MaaControllerHandle,
    contact: c_int,
    x: c_int,
    y: c_int,
    pressure: c_int,
) -> MaaCtrlId {
    type Fn = extern "C" fn(MaaControllerHandle, c_int, c_int, c_int, c_int) -> MaaCtrlId;
    load_symbol!(framework(), MaaControllerPostTouchDown, Fn)(ctrl, contact, x, y, pressure)
}

pub fn maa_controller_post_touch_move(
    ctrl: MaaControllerHandle,
    contact: c_int,
    x: c_int,
    y: c_int,
    pressure: c_int,
) -> MaaCtrlId {
    type Fn = extern "C" fn(MaaControllerHandle, c_int, c_int, c_int, c_int) -> MaaCtrlId;
    load_symbol!(framework(), MaaControllerPostTouchMove, Fn)(ctrl, contact, x, y, pressure)
}

pub fn maa_controller_post_touch_up(ctrl: MaaControllerHandle, contact: c_int) -> MaaCtrlId {
    type Fn = extern "C" fn(MaaControllerHandle, c_int) -> MaaCtrlId;
    load_symbol!(framework(), MaaControllerPostTouchUp, Fn)(ctrl, contact)
}

pub fn maa_controller_post_screencap(ctrl: MaaControllerHandle) -> MaaCtrlId {
    type Fn = extern "C" fn(MaaControllerHandle) -> MaaCtrlId;
    load_symbol!(framework(), MaaControllerPostScreencap, Fn)(ctrl)
}

pub fn maa_controller_post_scroll(ctrl: MaaControllerHandle, dx: c_int, dy: c_int) -> MaaCtrlId {
    type Fn = extern "C" fn(MaaControllerHandle, c_int, c_int) -> MaaCtrlId;
    load_symbol!(framework(), MaaControllerPostScroll, Fn)(ctrl, dx, dy)
}

pub fn maa_controller_status(ctrl: MaaControllerHandle, id: MaaCtrlId) -> MaaStatus {
    type Fn = extern "C" fn(MaaControllerHandle, MaaCtrlId) -> MaaStatus;
    load_symbol!(framework(), MaaControllerStatus, Fn)(ctrl, id)
}

pub fn maa_controller_wait(ctrl: MaaControllerHandle, id: MaaCtrlId) -> MaaStatus {
    type Fn = extern "C" fn(MaaControllerHandle, MaaCtrlId) -> MaaStatus;
    load_symbol!(framework(), MaaControllerWait, Fn)(ctrl, id)
}

pub fn maa_controller_connected(ctrl: MaaControllerHandle) -> MaaBool {
    type Fn = extern "C" fn(MaaControllerHandle) -> MaaBool;
    load_symbol!(framework(), MaaControllerConnected, Fn)(ctrl)
}

pub fn maa_controller_cached_image(
    ctrl: MaaControllerHandle,
    buffer: MaaImageBufferHandle,
) -> MaaBool {
    type Fn = extern "C" fn(MaaControllerHandle, MaaImageBufferHandle) -> MaaBool;
    load_symbol!(framework(), MaaControllerCachedImage, Fn)(ctrl, buffer)
}

pub fn maa_controller_get_uuid(
    ctrl: MaaControllerHandle,
    buffer: MaaStringBufferHandle,
) -> MaaBool {
    type Fn = extern "C" fn(MaaControllerHandle, MaaStringBufferHandle) -> MaaBool;
    load_symbol!(framework(), MaaControllerGetUuid, Fn)(ctrl, buffer)
}

// MaaTasker.h
pub fn maa_tasker_create() -> MaaTaskerHandle {
    type Fn = extern "C" fn() -> MaaTaskerHandle;
    load_symbol!(framework(), MaaTaskerCreate, Fn)()
}

pub fn maa_tasker_destroy(tasker: MaaTaskerHandle) {
    type Fn = extern "C" fn(MaaTaskerHandle);
    load_symbol!(framework(), MaaTaskerDestroy, Fn)(tasker)
}

pub fn maa_tasker_add_sink(
    tasker: MaaTaskerHandle,
    callback: MaaEventCallback,
    trans_arg: *mut c_void,
) -> MaaSinkId {
    type Fn = extern "C" fn(MaaTaskerHandle, MaaEventCallback, *mut c_void) -> MaaSinkId;
    load_symbol!(framework(), MaaTaskerAddSink, Fn)(tasker, callback, trans_arg)
}

pub fn maa_tasker_remove_sink(tasker: MaaTaskerHandle, sink_id: MaaSinkId) {
    type Fn = extern "C" fn(MaaTaskerHandle, MaaSinkId);
    load_symbol!(framework(), MaaTaskerRemoveSink, Fn)(tasker, sink_id)
}

pub fn maa_tasker_clear_sinks(tasker: MaaTaskerHandle) {
    type Fn = extern "C" fn(MaaTaskerHandle);
    load_symbol!(framework(), MaaTaskerClearSinks, Fn)(tasker)
}

pub fn maa_tasker_add_context_sink(
    tasker: MaaTaskerHandle,
    callback: MaaEventCallback,
    trans_arg: *mut c_void,
) -> MaaSinkId {
    type Fn = extern "C" fn(MaaTaskerHandle, MaaEventCallback, *mut c_void) -> MaaSinkId;
    load_symbol!(framework(), MaaTaskerAddContextSink, Fn)(tasker, callback, trans_arg)
}

pub fn maa_tasker_remove_context_sink(tasker: MaaTaskerHandle, sink_id: MaaSinkId) {
    type Fn = extern "C" fn(MaaTaskerHandle, MaaSinkId);
    load_symbol!(framework(), MaaTaskerRemoveContextSink, Fn)(tasker, sink_id)
}

pub fn maa_tasker_clear_context_sinks(tasker: MaaTaskerHandle) {
    type Fn = extern "C" fn(MaaTaskerHandle);
    load_symbol!(framework(), MaaTaskerClearContextSinks, Fn)(tasker)
}

pub fn maa_tasker_bind_resource(tasker: MaaTaskerHandle, res: MaaResourceHandle) -> MaaBool {
    type Fn = extern "C" fn(MaaTaskerHandle, MaaResourceHandle) -> MaaBool;
    load_symbol!(framework(), MaaTaskerBindResource, Fn)(tasker, res)
}

pub fn maa_tasker_bind_controller(tasker: MaaTaskerHandle, ctrl: MaaControllerHandle) -> MaaBool {
    type Fn = extern "C" fn(MaaTaskerHandle, MaaControllerHandle) -> MaaBool;
    load_symbol!(framework(), MaaTaskerBindController, Fn)(tasker, ctrl)
}

pub fn maa_tasker_inited(tasker: MaaTaskerHandle) -> MaaBool {
    type Fn = extern "C" fn(MaaTaskerHandle) -> MaaBool;
    load_symbol!(framework(), MaaTaskerInited, Fn)(tasker)
}

pub fn maa_tasker_post_task(
    tasker: MaaTaskerHandle,
    entry: *const c_char,
    pipeline_override: *const c_char,
) -> MaaTaskId {
    type Fn = extern "C" fn(MaaTaskerHandle, *const c_char, *const c_char) -> MaaTaskId;
    load_symbol!(framework(), MaaTaskerPostTask, Fn)(tasker, entry, pipeline_override)
}

pub fn maa_tasker_status(tasker: MaaTaskerHandle, id: MaaTaskId) -> MaaStatus {
    type Fn = extern "C" fn(MaaTaskerHandle, MaaTaskId) -> MaaStatus;
    load_symbol!(framework(), MaaTaskerStatus, Fn)(tasker, id)
}

pub fn maa_tasker_wait(tasker: MaaTaskerHandle, id: MaaTaskId) -> MaaStatus {
    type Fn = extern "C" fn(MaaTaskerHandle, MaaTaskId) -> MaaStatus;
    load_symbol!(framework(), MaaTaskerWait, Fn)(tasker, id)
}

pub fn maa_tasker_running(tasker: MaaTaskerHandle) -> MaaBool {
    type Fn = extern "C" fn(MaaTaskerHandle) -> MaaBool;
    load_symbol!(framework(), MaaTaskerRunning, Fn)(tasker)
}

pub fn maa_tasker_post_stop(tasker: MaaTaskerHandle) -> MaaTaskId {
    type Fn = extern "C" fn(MaaTaskerHandle) -> MaaTaskId;
    load_symbol!(framework(), MaaTaskerPostStop, Fn)(tasker)
}

pub fn maa_tasker_stopping(tasker: MaaTaskerHandle) -> MaaBool {
    type Fn = extern "C" fn(MaaTaskerHandle) -> MaaBool;
    load_symbol!(framework(), MaaTaskerStopping, Fn)(tasker)
}

pub fn maa_tasker_get_resource(tasker: MaaTaskerHandle) -> MaaResourceHandle {
    type Fn = extern "C" fn(MaaTaskerHandle) -> MaaResourceHandle;
    load_symbol!(framework(), MaaTaskerGetResource, Fn)(tasker)
}

pub fn maa_tasker_get_controller(tasker: MaaTaskerHandle) -> MaaControllerHandle {
    type Fn = extern "C" fn(MaaTaskerHandle) -> MaaControllerHandle;
    load_symbol!(framework(), MaaTaskerGetController, Fn)(tasker)
}

pub fn maa_tasker_get_recognition_detail(
    tasker: MaaTaskerHandle,
    reco_id: MaaRecoId,
    node_name: MaaStringBufferHandle,
    algorithm: MaaStringBufferHandle,
    hit: *mut MaaBool,
    box_: MaaRectHandle,
    detail_json: MaaStringBufferHandle,
    raw: MaaImageBufferHandle,
    draws: MaaImageListBufferHandle,
) -> MaaBool {
    type Fn = extern "C" fn(
        MaaTaskerHandle,
        MaaRecoId,
        MaaStringBufferHandle,
        MaaStringBufferHandle,
        *mut MaaBool,
        MaaRectHandle,
        MaaStringBufferHandle,
        MaaImageBufferHandle,
        MaaImageListBufferHandle,
    ) -> MaaBool;
    load_symbol!(framework(), MaaTaskerGetRecognitionDetail, Fn)(
        tasker, reco_id, node_name, algorithm, hit, box_, detail_json, raw, draws,
    )
}

pub fn maa_tasker_get_action_detail(
    tasker: MaaTaskerHandle,
    action_id: MaaActId,
    node_name: MaaStringBufferHandle,
    action: MaaStringBufferHandle,
    box_: MaaRectHandle,
    success: *mut MaaBool,
    detail_json: MaaStringBufferHandle,
) -> MaaBool {
    type Fn = extern "C" fn(
        MaaTaskerHandle,
        MaaActId,
        MaaStringBufferHandle,
        MaaStringBufferHandle,
        MaaRectHandle,
        *mut MaaBool,
        MaaStringBufferHandle,
    ) -> MaaBool;
    load_symbol!(framework(), MaaTaskerGetActionDetail, Fn)(
        tasker, action_id, node_name, action, box_, success, detail_json,
    )
}

pub fn maa_tasker_get_node_detail(
    tasker: MaaTaskerHandle,
    node_id: MaaNodeId,
    node_name: MaaStringBufferHandle,
    reco_id: *mut MaaRecoId,
    action_id: *mut MaaActId,
    completed: *mut MaaBool,
) -> MaaBool {
    type Fn = extern "C" fn(
        MaaTaskerHandle,
        MaaNodeId,
        MaaStringBufferHandle,
        *mut MaaRecoId,
        *mut MaaActId,
        *mut MaaBool,
    ) -> MaaBool;
    load_symbol!(framework(), MaaTaskerGetNodeDetail, Fn)(
        tasker, node_id, node_name, reco_id, action_id, completed,
    )
}

pub fn maa_tasker_get_task_detail(
    tasker: MaaTaskerHandle,
    task_id: MaaTaskId,
    entry: MaaStringBufferHandle,
    node_id_list: *mut MaaNodeId,
    node_id_list_size: *mut MaaSize,
    status: *mut MaaStatus,
) -> MaaBool {
    type Fn = extern "C" fn(
        MaaTaskerHandle,
        MaaTaskId,
        MaaStringBufferHandle,
        *mut MaaNodeId,
        *mut MaaSize,
        *mut MaaStatus,
    ) -> MaaBool;
    load_symbol!(framework(), MaaTaskerGetTaskDetail, Fn)(
        tasker,
        task_id,
        entry,
        node_id_list,
        node_id_list_size,
        status,
    )
}

pub fn maa_tasker_get_latest_node(
    tasker: MaaTaskerHandle,
    node_name: *const c_char,
    latest_id: *mut MaaNodeId,
) -> MaaBool {
    type Fn = extern "C" fn(MaaTaskerHandle, *const c_char, *mut MaaNodeId) -> MaaBool;
    load_symbol!(framework(), MaaTaskerGetLatestNode, Fn)(tasker, node_name, latest_id)
}

pub fn maa_tasker_clear_cache(tasker: MaaTaskerHandle) -> MaaBool {
    type Fn = extern "C" fn(MaaTaskerHandle) -> MaaBool;
    load_symbol!(framework(), MaaTaskerClearCache, Fn)(tasker)
}

// MaaContext.h
pub fn maa_context_run_task(
    ctx: MaaContextHandle,
    entry: *const c_char,
    pipeline_override: *const c_char,
) -> MaaTaskId {
    type Fn = extern "C" fn(MaaContextHandle, *const c_char, *const c_char) -> MaaTaskId;
    load_symbol!(framework(), MaaContextRunTask, Fn)(ctx, entry, pipeline_override)
}

pub fn maa_context_run_recognition(
    ctx: MaaContextHandle,
    entry: *const c_char,
    pipeline_override: *const c_char,
    image: MaaImageBufferHandle,
) -> MaaRecoId {
    type Fn = extern "C" fn(
        MaaContextHandle,
        *const c_char,
        *const c_char,
        MaaImageBufferHandle,
    ) -> MaaRecoId;
    load_symbol!(framework(), MaaContextRunRecognition, Fn)(ctx, entry, pipeline_override, image)
}

pub fn maa_context_run_action(
    ctx: MaaContextHandle,
    entry: *const c_char,
    pipeline_override: *const c_char,
    box_: MaaRectHandle,
    reco_detail: *const c_char,
) -> MaaActId {
    type Fn = extern "C" fn(
        MaaContextHandle,
        *const c_char,
        *const c_char,
        MaaRectHandle,
        *const c_char,
    ) -> MaaActId;
    load_symbol!(framework(), MaaContextRunAction, Fn)(
        ctx,
        entry,
        pipeline_override,
        box_,
        reco_detail,
    )
}

pub fn maa_context_override_pipeline(
    ctx: MaaContextHandle,
    pipeline_override: *const c_char,
) -> MaaBool {
    type Fn = extern "C" fn(MaaContextHandle, *const c_char) -> MaaBool;
    load_symbol!(framework(), MaaContextOverridePipeline, Fn)(ctx, pipeline_override)
}

pub fn maa_context_override_next(
    ctx: MaaContextHandle,
    node_name: *const c_char,
    next_list: MaaStringListBufferHandle,
) -> MaaBool {
    type Fn = extern "C" fn(MaaContextHandle, *const c_char, MaaStringListBufferHandle) -> MaaBool;
    load_symbol!(framework(), MaaContextOverrideNext, Fn)(ctx, node_name, next_list)
}

pub fn maa_context_override_image(
    ctx: MaaContextHandle,
    image_name: *const c_char,
    image: MaaImageBufferHandle,
) -> MaaBool {
    type Fn = extern "C" fn(MaaContextHandle, *const c_char, MaaImageBufferHandle) -> MaaBool;
    load_symbol!(framework(), MaaContextOverrideImage, Fn)(ctx, image_name, image)
}

pub fn maa_context_get_node_data(
    ctx: MaaContextHandle,
    node_name: *const c_char,
    buffer: MaaStringBufferHandle,
) -> MaaBool {
    type Fn = extern "C" fn(MaaContextHandle, *const c_char, MaaStringBufferHandle) -> MaaBool;
    load_symbol!(framework(), MaaContextGetNodeData, Fn)(ctx, node_name, buffer)
}

pub fn maa_context_get_task_id(ctx: MaaContextHandle) -> MaaTaskId {
    type Fn = extern "C" fn(MaaContextHandle) -> MaaTaskId;
    load_symbol!(framework(), MaaContextGetTaskId, Fn)(ctx)
}

pub fn maa_context_get_tasker(ctx: MaaContextHandle) -> MaaTaskerHandle {
    type Fn = extern "C" fn(MaaContextHandle) -> MaaTaskerHandle;
    load_symbol!(framework(), MaaContextGetTasker, Fn)(ctx)
}

pub fn maa_context_clone(ctx: MaaContextHandle) -> MaaContextHandle {
    type Fn = extern "C" fn(MaaContextHandle) -> MaaContextHandle;
    load_symbol!(framework(), MaaContextClone, Fn)(ctx)
}

pub fn maa_context_set_anchor(
    ctx: MaaContextHandle,
    anchor_name: *const c_char,
    node_name: *const c_char,
) -> MaaBool {
    type Fn = extern "C" fn(MaaContextHandle, *const c_char, *const c_char) -> MaaBool;
    load_symbol!(framework(), MaaContextSetAnchor, Fn)(ctx, anchor_name, node_name)
}

pub fn maa_context_get_anchor(
    ctx: MaaContextHandle,
    anchor_name: *const c_char,
    buffer: MaaStringBufferHandle,
) -> MaaBool {
    type Fn = extern "C" fn(MaaContextHandle, *const c_char, MaaStringBufferHandle) -> MaaBool;
    load_symbol!(framework(), MaaContextGetAnchor, Fn)(ctx, anchor_name, buffer)
}

pub fn maa_context_get_hit_count(
    ctx: MaaContextHandle,
    node_name: *const c_char,
    count: *mut u64,
) -> MaaBool {
    type Fn = extern "C" fn(MaaContextHandle, *const c_char, *mut u64) -> MaaBool;
    load_symbol!(framework(), MaaContextGetHitCount, Fn)(ctx, node_name, count)
}

pub fn maa_context_clear_hit_count(ctx: MaaContextHandle, node_name: *const c_char) -> MaaBool {
    type Fn = extern "C" fn(MaaContextHandle, *const c_char) -> MaaBool;
    load_symbol!(framework(), MaaContextClearHitCount, Fn)(ctx, node_name)
}

// MaaToolkitConfig.h
pub fn maa_toolkit_config_init_option(
    user_path: *const c_char,
    default_json: *const c_char,
) -> MaaBool {
    type Fn = extern "C" fn(*const c_char, *const c_char) -> MaaBool;
    load_symbol!(toolkit(), MaaToolkitConfigInitOption, Fn)(user_path, default_json)
}

// MaaToolkitAdbDevice.h
pub type MaaToolkitAdbDeviceListHandle = *mut c_void;
pub type MaaToolkitAdbDeviceHandle = *mut c_void;

pub fn maa_toolkit_adb_device_list_create() -> MaaToolkitAdbDeviceListHandle {
    type Fn = extern "C" fn() -> MaaToolkitAdbDeviceListHandle;
    load_symbol!(toolkit(), MaaToolkitAdbDeviceListCreate, Fn)()
}

pub fn maa_toolkit_adb_device_list_destroy(handle: MaaToolkitAdbDeviceListHandle) {
    type Fn = extern "C" fn(MaaToolkitAdbDeviceListHandle);
    load_symbol!(toolkit(), MaaToolkitAdbDeviceListDestroy, Fn)(handle)
}

pub fn maa_toolkit_adb_device_find(buffer: MaaToolkitAdbDeviceListHandle) -> MaaBool {
    type Fn = extern "C" fn(MaaToolkitAdbDeviceListHandle) -> MaaBool;
    load_symbol!(toolkit(), MaaToolkitAdbDeviceFind, Fn)(buffer)
}

pub fn maa_toolkit_adb_device_find_specified(
    adb_path: *const c_char,
    buffer: MaaToolkitAdbDeviceListHandle,
) -> MaaBool {
    type Fn = extern "C" fn(*const c_char, MaaToolkitAdbDeviceListHandle) -> MaaBool;
    load_symbol!(toolkit(), MaaToolkitAdbDeviceFindSpecified, Fn)(adb_path, buffer)
}

pub fn maa_toolkit_adb_device_list_size(list: MaaToolkitAdbDeviceListHandle) -> MaaSize {
    type Fn = extern "C" fn(MaaToolkitAdbDeviceListHandle) -> MaaSize;
    load_symbol!(toolkit(), MaaToolkitAdbDeviceListSize, Fn)(list)
}

pub fn maa_toolkit_adb_device_list_at(
    list: MaaToolkitAdbDeviceListHandle,
    index: MaaSize,
) -> MaaToolkitAdbDeviceHandle {
    type Fn = extern "C" fn(MaaToolkitAdbDeviceListHandle, MaaSize) -> MaaToolkitAdbDeviceHandle;
    load_symbol!(toolkit(), MaaToolkitAdbDeviceListAt, Fn)(list, index)
}

pub fn maa_toolkit_adb_device_get_name(device: MaaToolkitAdbDeviceHandle) -> *const c_char {
    type Fn = extern "C" fn(MaaToolkitAdbDeviceHandle) -> *const c_char;
    load_symbol!(toolkit(), MaaToolkitAdbDeviceGetName, Fn)(device)
}

pub fn maa_toolkit_adb_device_get_adb_path(device: MaaToolkitAdbDeviceHandle) -> *const c_char {
    type Fn = extern "C" fn(MaaToolkitAdbDeviceHandle) -> *const c_char;
    load_symbol!(toolkit(), MaaToolkitAdbDeviceGetAdbPath, Fn)(device)
}

pub fn maa_toolkit_adb_device_get_address(device: MaaToolkitAdbDeviceHandle) -> *const c_char {
    type Fn = extern "C" fn(MaaToolkitAdbDeviceHandle) -> *const c_char;
    load_symbol!(toolkit(), MaaToolkitAdbDeviceGetAddress, Fn)(device)
}

pub fn maa_toolkit_adb_device_get_screencap_methods(
    device: MaaToolkitAdbDeviceHandle,
) -> MaaAdbScreencapMethod {
    type Fn = extern "C" fn(MaaToolkitAdbDeviceHandle) -> MaaAdbScreencapMethod;
    load_symbol!(toolkit(), MaaToolkitAdbDeviceGetScreencapMethods, Fn)(device)
}

pub fn maa_toolkit_adb_device_get_input_methods(
    device: MaaToolkitAdbDeviceHandle,
) -> MaaAdbInputMethod {
    type Fn = extern "C" fn(MaaToolkitAdbDeviceHandle) -> MaaAdbInputMethod;
    load_symbol!(toolkit(), MaaToolkitAdbDeviceGetInputMethods, Fn)(device)
}

pub fn maa_toolkit_adb_device_get_config(device: MaaToolkitAdbDeviceHandle) -> *const c_char {
    type Fn = extern "C" fn(MaaToolkitAdbDeviceHandle) -> *const c_char;
    load_symbol!(toolkit(), MaaToolkitAdbDeviceGetConfig, Fn)(device)
}

// MaaToolkitDesktopWindow.h
pub type MaaToolkitDesktopWindowListHandle = *mut c_void;
pub type MaaToolkitDesktopWindowHandle = *mut c_void;

pub fn maa_toolkit_desktop_window_list_create() -> MaaToolkitDesktopWindowListHandle {
    type Fn = extern "C" fn() -> MaaToolkitDesktopWindowListHandle;
    load_symbol!(toolkit(), MaaToolkitDesktopWindowListCreate, Fn)()
}

pub fn maa_toolkit_desktop_window_list_destroy(handle: MaaToolkitDesktopWindowListHandle) {
    type Fn = extern "C" fn(MaaToolkitDesktopWindowListHandle);
    load_symbol!(toolkit(), MaaToolkitDesktopWindowListDestroy, Fn)(handle)
}

pub fn maa_toolkit_desktop_window_find_all(buffer: MaaToolkitDesktopWindowListHandle) -> MaaBool {
    type Fn = extern "C" fn(MaaToolkitDesktopWindowListHandle) -> MaaBool;
    load_symbol!(toolkit(), MaaToolkitDesktopWindowFindAll, Fn)(buffer)
}

pub fn maa_toolkit_desktop_window_list_size(list: MaaToolkitDesktopWindowListHandle) -> MaaSize {
    type Fn = extern "C" fn(MaaToolkitDesktopWindowListHandle) -> MaaSize;
    load_symbol!(toolkit(), MaaToolkitDesktopWindowListSize, Fn)(list)
}

pub fn maa_toolkit_desktop_window_list_at(
    list: MaaToolkitDesktopWindowListHandle,
    index: MaaSize,
) -> MaaToolkitDesktopWindowHandle {
    type Fn =
        extern "C" fn(MaaToolkitDesktopWindowListHandle, MaaSize) -> MaaToolkitDesktopWindowHandle;
    load_symbol!(toolkit(), MaaToolkitDesktopWindowListAt, Fn)(list, index)
}

pub fn maa_toolkit_desktop_window_get_handle(window: MaaToolkitDesktopWindowHandle) -> *mut c_void {
    type Fn = extern "C" fn(MaaToolkitDesktopWindowHandle) -> *mut c_void;
    load_symbol!(toolkit(), MaaToolkitDesktopWindowGetHandle, Fn)(window)
}

pub fn maa_toolkit_desktop_window_get_class_name(
    window: MaaToolkitDesktopWindowHandle,
) -> *const c_char {
    type Fn = extern "C" fn(MaaToolkitDesktopWindowHandle) -> *const c_char;
    load_symbol!(toolkit(), MaaToolkitDesktopWindowGetClassName, Fn)(window)
}

pub fn maa_toolkit_desktop_window_get_window_name(
    window: MaaToolkitDesktopWindowHandle,
) -> *const c_char {
    type Fn = extern "C" fn(MaaToolkitDesktopWindowHandle) -> *const c_char;
    load_symbol!(toolkit(), MaaToolkitDesktopWindowGetWindowName, Fn)(window)
}

