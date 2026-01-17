use crate::sys;
use std::ffi::CStr;
use std::os::raw::c_void;

/// Custom controller callback trait for implementing custom device control.
pub trait CustomControllerCallback: Send + Sync {
    fn connect(&self) -> bool;
    fn connected(&self) -> bool {
        true
    }
    fn request_uuid(&self) -> Option<String> {
        None
    }
    fn get_features(&self) -> u64 {
        0
    }
    fn start_app(&self, _intent: &str) -> bool {
        false
    }
    fn stop_app(&self, _intent: &str) -> bool {
        false
    }
    /// Returns PNG-encoded screenshot data.
    fn screencap(&self) -> Option<Vec<u8>> {
        None
    }
    fn click(&self, _x: i32, _y: i32) -> bool {
        false
    }
    fn swipe(&self, _x1: i32, _y1: i32, _x2: i32, _y2: i32, _duration: i32) -> bool {
        false
    }
    fn touch_down(&self, _contact: i32, _x: i32, _y: i32, _pressure: i32) -> bool {
        false
    }
    fn touch_move(&self, _contact: i32, _x: i32, _y: i32, _pressure: i32) -> bool {
        false
    }
    fn touch_up(&self, _contact: i32) -> bool {
        false
    }
    fn click_key(&self, _keycode: i32) -> bool {
        false
    }
    fn input_text(&self, _text: &str) -> bool {
        false
    }
    fn key_down(&self, _keycode: i32) -> bool {
        false
    }
    fn key_up(&self, _keycode: i32) -> bool {
        false
    }
    fn scroll(&self, _dx: i32, _dy: i32) -> bool {
        false
    }
}

type BoxedCallback = Box<dyn CustomControllerCallback>;

// FFI trampolines
unsafe extern "C" fn connect_trampoline(trans_arg: *mut c_void) -> sys::MaaBool {
    let cb = &*(trans_arg as *const BoxedCallback);
    if cb.connect() {
        1
    } else {
        0
    }
}

unsafe extern "C" fn connected_trampoline(trans_arg: *mut c_void) -> sys::MaaBool {
    let cb = &*(trans_arg as *const BoxedCallback);
    if cb.connected() {
        1
    } else {
        0
    }
}

unsafe extern "C" fn request_uuid_trampoline(
    trans_arg: *mut c_void,
    buffer: *mut sys::MaaStringBuffer,
) -> sys::MaaBool {
    let cb = &*(trans_arg as *const BoxedCallback);
    if let Some(uuid) = cb.request_uuid() {
        if let Ok(c_str) = std::ffi::CString::new(uuid) {
            sys::MaaStringBufferSetEx(buffer, c_str.as_ptr(), c_str.as_bytes().len() as u64);
            return 1;
        }
    }
    0
}

unsafe extern "C" fn get_features_trampoline(trans_arg: *mut c_void) -> sys::MaaControllerFeature {
    let cb = &*(trans_arg as *const BoxedCallback);
    cb.get_features()
}

unsafe extern "C" fn start_app_trampoline(
    intent: *const std::os::raw::c_char,
    trans_arg: *mut c_void,
) -> sys::MaaBool {
    let cb = &*(trans_arg as *const BoxedCallback);
    let intent_str = if !intent.is_null() {
        CStr::from_ptr(intent).to_string_lossy()
    } else {
        std::borrow::Cow::Borrowed("")
    };
    if cb.start_app(&intent_str) {
        1
    } else {
        0
    }
}

unsafe extern "C" fn stop_app_trampoline(
    intent: *const std::os::raw::c_char,
    trans_arg: *mut c_void,
) -> sys::MaaBool {
    let cb = &*(trans_arg as *const BoxedCallback);
    let intent_str = if !intent.is_null() {
        CStr::from_ptr(intent).to_string_lossy()
    } else {
        std::borrow::Cow::Borrowed("")
    };
    if cb.stop_app(&intent_str) {
        1
    } else {
        0
    }
}

unsafe extern "C" fn screencap_trampoline(
    trans_arg: *mut c_void,
    buffer: *mut sys::MaaImageBuffer,
) -> sys::MaaBool {
    let cb = &*(trans_arg as *const BoxedCallback);
    if let Some(data) = cb.screencap() {
        let ret =
            sys::MaaImageBufferSetEncoded(buffer, data.as_ptr() as *mut u8, data.len() as u64);
        return ret;
    }
    0
}

unsafe extern "C" fn click_trampoline(x: i32, y: i32, trans_arg: *mut c_void) -> sys::MaaBool {
    let cb = &*(trans_arg as *const BoxedCallback);
    if cb.click(x, y) {
        1
    } else {
        0
    }
}

unsafe extern "C" fn swipe_trampoline(
    x1: i32,
    y1: i32,
    x2: i32,
    y2: i32,
    duration: i32,
    trans_arg: *mut c_void,
) -> sys::MaaBool {
    let cb = &*(trans_arg as *const BoxedCallback);
    if cb.swipe(x1, y1, x2, y2, duration) {
        1
    } else {
        0
    }
}

unsafe extern "C" fn touch_down_trampoline(
    contact: i32,
    x: i32,
    y: i32,
    pressure: i32,
    trans_arg: *mut c_void,
) -> sys::MaaBool {
    let cb = &*(trans_arg as *const BoxedCallback);
    if cb.touch_down(contact, x, y, pressure) {
        1
    } else {
        0
    }
}

unsafe extern "C" fn touch_move_trampoline(
    contact: i32,
    x: i32,
    y: i32,
    pressure: i32,
    trans_arg: *mut c_void,
) -> sys::MaaBool {
    let cb = &*(trans_arg as *const BoxedCallback);
    if cb.touch_move(contact, x, y, pressure) {
        1
    } else {
        0
    }
}

unsafe extern "C" fn touch_up_trampoline(contact: i32, trans_arg: *mut c_void) -> sys::MaaBool {
    let cb = &*(trans_arg as *const BoxedCallback);
    if cb.touch_up(contact) {
        1
    } else {
        0
    }
}

unsafe extern "C" fn click_key_trampoline(keycode: i32, trans_arg: *mut c_void) -> sys::MaaBool {
    let cb = &*(trans_arg as *const BoxedCallback);
    if cb.click_key(keycode) {
        1
    } else {
        0
    }
}

unsafe extern "C" fn input_text_trampoline(
    text: *const std::os::raw::c_char,
    trans_arg: *mut c_void,
) -> sys::MaaBool {
    let cb = &*(trans_arg as *const BoxedCallback);
    let text_str = if !text.is_null() {
        CStr::from_ptr(text).to_string_lossy()
    } else {
        std::borrow::Cow::Borrowed("")
    };
    if cb.input_text(&text_str) {
        1
    } else {
        0
    }
}

unsafe extern "C" fn key_down_trampoline(keycode: i32, trans_arg: *mut c_void) -> sys::MaaBool {
    let cb = &*(trans_arg as *const BoxedCallback);
    if cb.key_down(keycode) {
        1
    } else {
        0
    }
}

unsafe extern "C" fn key_up_trampoline(keycode: i32, trans_arg: *mut c_void) -> sys::MaaBool {
    let cb = &*(trans_arg as *const BoxedCallback);
    if cb.key_up(keycode) {
        1
    } else {
        0
    }
}

unsafe extern "C" fn scroll_trampoline(dx: i32, dy: i32, trans_arg: *mut c_void) -> sys::MaaBool {
    let cb = &*(trans_arg as *const BoxedCallback);
    if cb.scroll(dx, dy) {
        1
    } else {
        0
    }
}

pub fn create_custom_controller_callbacks() -> sys::MaaCustomControllerCallbacks {
    sys::MaaCustomControllerCallbacks {
        connect: Some(connect_trampoline),
        connected: Some(connected_trampoline),
        request_uuid: Some(request_uuid_trampoline),
        get_features: Some(get_features_trampoline),
        start_app: Some(start_app_trampoline),
        stop_app: Some(stop_app_trampoline),
        screencap: Some(screencap_trampoline),
        click: Some(click_trampoline),
        swipe: Some(swipe_trampoline),
        touch_down: Some(touch_down_trampoline),
        touch_move: Some(touch_move_trampoline),
        touch_up: Some(touch_up_trampoline),
        click_key: Some(click_key_trampoline),
        input_text: Some(input_text_trampoline),
        key_down: Some(key_down_trampoline),
        key_up: Some(key_up_trampoline),
        scroll: Some(scroll_trampoline),
    }
}

static CALLBACKS: std::sync::OnceLock<sys::MaaCustomControllerCallbacks> =
    std::sync::OnceLock::new();

pub fn get_callbacks() -> &'static sys::MaaCustomControllerCallbacks {
    CALLBACKS.get_or_init(create_custom_controller_callbacks)
}
