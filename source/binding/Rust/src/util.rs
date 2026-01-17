use crate::{common, sys, MaaResult};
use std::ffi::{CStr, CString};

pub fn version() -> String {
    unsafe {
        let ptr = sys::MaaVersion();
        if ptr.is_null() {
            String::new()
        } else {
            CStr::from_ptr(ptr).to_string_lossy().into_owned()
        }
    }
}

pub fn load_plugin(path: &str) -> MaaResult<()> {
    let c_path = CString::new(path)?;
    let ret = unsafe { sys::MaaGlobalLoadPlugin(c_path.as_ptr()) };
    common::check_bool(ret)
}

pub mod logging {
    use super::*;

    pub fn set_log_dir(path: &str) -> MaaResult<()> {
        let c_path = CString::new(path)?;
        let len = c_path.as_bytes().len();
        unsafe {
            let ret = sys::MaaGlobalSetOption(
                sys::MaaGlobalOptionEnum_MaaGlobalOption_LogDir as i32,
                c_path.as_ptr() as *mut _,
                len as u64,
            );
            common::check_bool(ret)
        }
    }
}
