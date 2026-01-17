use crate::sys;
use std::ffi::CStr;
use std::os::raw::c_void;

pub type EventCallbackFn = Box<dyn Fn(&str, &str) + Send + Sync>;

unsafe extern "C" fn event_callback_trampoline(
    _handle: *mut c_void,
    msg: *const std::os::raw::c_char,
    details: *const std::os::raw::c_char,
    trans_arg: *mut c_void,
) {
    if trans_arg.is_null() {
        return;
    }
    let callback = &*(trans_arg as *const EventCallbackFn);

    let msg_str = if !msg.is_null() {
        CStr::from_ptr(msg).to_string_lossy()
    } else {
        std::borrow::Cow::Borrowed("")
    };

    let details_str = if !details.is_null() {
        CStr::from_ptr(details).to_string_lossy()
    } else {
        std::borrow::Cow::Borrowed("")
    };

    callback(&msg_str, &details_str);
}

pub struct EventCallback {
    _ptr: *mut c_void,
}

impl EventCallback {
    pub fn new(
        cb: impl Fn(&str, &str) + Send + Sync + 'static,
    ) -> (sys::MaaEventCallback, *mut c_void) {
        let boxed: EventCallbackFn = Box::new(cb);
        let ptr = Box::into_raw(Box::new(boxed)) as *mut c_void;
        (Some(event_callback_trampoline), ptr)
    }

    pub unsafe fn drop_callback(ptr: *mut c_void) {
        if !ptr.is_null() {
            let _ = Box::from_raw(ptr as *mut EventCallbackFn);
        }
    }
}
