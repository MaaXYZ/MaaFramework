use crate::define::*;
use crate::error::{MaaError, Result};
use crate::ffi;
use serde_json::Value;
use std::ffi::{c_void, CStr, CString};
use std::path::{Path, PathBuf};

#[derive(Debug, Clone)]
pub struct AdbDevice {
    pub name: String,
    pub adb_path: PathBuf,
    pub address: String,
    pub screencap_methods: MaaAdbScreencapMethod,
    pub input_methods: MaaAdbInputMethod,
    pub config: Value,
}

/// Represents a desktop window handle and its properties.
///
/// # Thread Safety
/// The hwnd is a raw pointer to a window handle. While the pointer itself
/// can be sent across threads, actual window operations may have thread
/// affinity requirements depending on the platform.
#[derive(Debug, Clone)]
pub struct DesktopWindow {
    pub hwnd: *mut c_void,
    pub class_name: String,
    pub window_name: String,
}

// SAFETY: The hwnd is just an opaque handle identifier.
// Actual window operations should respect platform threading rules.
unsafe impl Send for DesktopWindow {}
unsafe impl Sync for DesktopWindow {}

pub struct Toolkit;

impl Toolkit {
    pub fn init_option(user_path: impl AsRef<Path>, default_config: &Value) -> Result<bool> {
        let path_str = user_path.as_ref().to_string_lossy();
        let path_cstr =
            CString::new(path_str.as_ref()).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let config_str = serde_json::to_string(default_config)?;
        let config_cstr =
            CString::new(config_str).map_err(|e| MaaError::BufferError(e.to_string()))?;

        Ok(ffi::maa_toolkit_config_init_option(path_cstr.as_ptr(), config_cstr.as_ptr()) != 0)
    }

    pub fn find_adb_devices() -> Vec<AdbDevice> {
        Self::find_adb_devices_impl(None)
    }

    pub fn find_adb_devices_specified(adb_path: impl AsRef<Path>) -> Result<Vec<AdbDevice>> {
        let path_str = adb_path.as_ref().to_string_lossy();
        let path_cstr =
            CString::new(path_str.as_ref()).map_err(|e| MaaError::BufferError(e.to_string()))?;
        Ok(Self::find_adb_devices_impl_cstr(Some(&path_cstr)))
    }

    fn find_adb_devices_impl(specified_adb: Option<&Path>) -> Vec<AdbDevice> {
        let cstr = specified_adb.and_then(|p| {
            let path_str = p.to_string_lossy();
            CString::new(path_str.as_ref()).ok()
        });
        Self::find_adb_devices_impl_cstr(cstr.as_ref())
    }

    fn find_adb_devices_impl_cstr(specified_adb: Option<&CString>) -> Vec<AdbDevice> {
        let list_handle = ffi::maa_toolkit_adb_device_list_create();

        let ok = if let Some(path_cstr) = specified_adb {
            ffi::maa_toolkit_adb_device_find_specified(path_cstr.as_ptr(), list_handle)
        } else {
            ffi::maa_toolkit_adb_device_find(list_handle)
        };

        // If discovery failed, clean up and return empty
        if ok == 0 {
            ffi::maa_toolkit_adb_device_list_destroy(list_handle);
            return Vec::new();
        }

        let count = ffi::maa_toolkit_adb_device_list_size(list_handle);
        let mut devices = Vec::with_capacity(count as usize);

        for i in 0..count {
            let device_handle = ffi::maa_toolkit_adb_device_list_at(list_handle, i);

            let name_ptr = ffi::maa_toolkit_adb_device_get_name(device_handle);
            let name = if name_ptr.is_null() {
                String::new()
            } else {
                unsafe { CStr::from_ptr(name_ptr) }
                    .to_string_lossy()
                    .into_owned()
            };

            let adb_path_ptr = ffi::maa_toolkit_adb_device_get_adb_path(device_handle);
            let adb_path = if adb_path_ptr.is_null() {
                PathBuf::new()
            } else {
                PathBuf::from(
                    unsafe { CStr::from_ptr(adb_path_ptr) }
                        .to_string_lossy()
                        .into_owned(),
                )
            };

            let address_ptr = ffi::maa_toolkit_adb_device_get_address(device_handle);
            let address = if address_ptr.is_null() {
                String::new()
            } else {
                unsafe { CStr::from_ptr(address_ptr) }
                    .to_string_lossy()
                    .into_owned()
            };

            let screencap_methods =
                ffi::maa_toolkit_adb_device_get_screencap_methods(device_handle);
            let input_methods = ffi::maa_toolkit_adb_device_get_input_methods(device_handle);

            let config_ptr = ffi::maa_toolkit_adb_device_get_config(device_handle);
            let config = if config_ptr.is_null() {
                Value::Object(Default::default())
            } else {
                let config_str = unsafe { CStr::from_ptr(config_ptr) }
                    .to_string_lossy()
                    .into_owned();
                serde_json::from_str(&config_str).unwrap_or(Value::Object(Default::default()))
            };

            devices.push(AdbDevice {
                name,
                adb_path,
                address,
                screencap_methods,
                input_methods,
                config,
            });
        }

        ffi::maa_toolkit_adb_device_list_destroy(list_handle);
        devices
    }

    pub fn find_desktop_windows() -> Vec<DesktopWindow> {
        let list_handle = ffi::maa_toolkit_desktop_window_list_create();
        let ok = ffi::maa_toolkit_desktop_window_find_all(list_handle);

        // If discovery failed, clean up and return empty
        if ok == 0 {
            ffi::maa_toolkit_desktop_window_list_destroy(list_handle);
            return Vec::new();
        }

        let count = ffi::maa_toolkit_desktop_window_list_size(list_handle);
        let mut windows = Vec::with_capacity(count as usize);

        for i in 0..count {
            let window_handle = ffi::maa_toolkit_desktop_window_list_at(list_handle, i);

            let hwnd = ffi::maa_toolkit_desktop_window_get_handle(window_handle);

            let class_name_ptr = ffi::maa_toolkit_desktop_window_get_class_name(window_handle);
            let class_name = if class_name_ptr.is_null() {
                String::new()
            } else {
                unsafe { CStr::from_ptr(class_name_ptr) }
                    .to_string_lossy()
                    .into_owned()
            };

            let window_name_ptr = ffi::maa_toolkit_desktop_window_get_window_name(window_handle);
            let window_name = if window_name_ptr.is_null() {
                String::new()
            } else {
                unsafe { CStr::from_ptr(window_name_ptr) }
                    .to_string_lossy()
                    .into_owned()
            };

            windows.push(DesktopWindow {
                hwnd,
                class_name,
                window_name,
            });
        }

        ffi::maa_toolkit_desktop_window_list_destroy(list_handle);
        windows
    }
}
