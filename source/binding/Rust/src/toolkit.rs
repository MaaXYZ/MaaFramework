use crate::{common, sys, MaaError, MaaResult};
use std::ffi::{CStr, CString};
use std::path::PathBuf;

#[derive(Debug, Clone)]
pub struct AdbDevice {
    pub name: String,
    pub adb_path: PathBuf,
    pub address: String,
    pub screencap_methods: u64,
    pub input_methods: u64,
    pub config: serde_json::Value,
}

#[derive(Debug, Clone)]
pub struct DesktopWindow {
    pub hwnd: *mut std::ffi::c_void,
    pub class_name: String,
    pub window_name: String,
}

pub struct Toolkit;

impl Toolkit {
    pub fn init_option(user_path: &str, default_config: &str) -> MaaResult<()> {
        let c_path = CString::new(user_path)?;
        let c_config = CString::new(default_config)?;
        let ret = unsafe { sys::MaaToolkitConfigInitOption(c_path.as_ptr(), c_config.as_ptr()) };
        common::check_bool(ret)
    }

    /// Find connected ADB devices.
    ///
    /// Scans for all known Android emulators and connected ADB devices.
    ///
    /// # Returns
    /// List of discovered ADB devices with their configurations.
    pub fn find_adb_devices() -> MaaResult<Vec<AdbDevice>> {
        Self::find_adb_devices_impl(None)
    }

    /// Find connected ADB devices using a specific ADB binary.
    ///
    /// # Arguments
    /// * `adb_path` - Path to the ADB binary to use for discovery
    ///
    /// # Returns
    /// List of discovered ADB devices with their configurations.
    pub fn find_adb_devices_with_adb(adb_path: &str) -> MaaResult<Vec<AdbDevice>> {
        Self::find_adb_devices_impl(Some(adb_path))
    }

    fn find_adb_devices_impl(specified_adb: Option<&str>) -> MaaResult<Vec<AdbDevice>> {
        let list = unsafe { sys::MaaToolkitAdbDeviceListCreate() };
        if list.is_null() {
            return Err(MaaError::NullPointer);
        }

        let _guard = AdbDeviceListGuard(list);

        unsafe {
            let ret = if let Some(adb_path) = specified_adb {
                let c_path = CString::new(adb_path)?;
                sys::MaaToolkitAdbDeviceFindSpecified(c_path.as_ptr(), list)
            } else {
                sys::MaaToolkitAdbDeviceFind(list)
            };
            common::check_bool(ret)?;

            let count = sys::MaaToolkitAdbDeviceListSize(list);
            let mut devices = Vec::with_capacity(count as usize);

            for i in 0..count {
                let device_ptr = sys::MaaToolkitAdbDeviceListAt(list, i);
                if device_ptr.is_null() {
                    continue;
                }

                let name = CStr::from_ptr(sys::MaaToolkitAdbDeviceGetName(device_ptr))
                    .to_string_lossy()
                    .into_owned();

                let adb_path_str = CStr::from_ptr(sys::MaaToolkitAdbDeviceGetAdbPath(device_ptr))
                    .to_string_lossy()
                    .into_owned();

                let address = CStr::from_ptr(sys::MaaToolkitAdbDeviceGetAddress(device_ptr))
                    .to_string_lossy()
                    .into_owned();

                let screencap_methods = sys::MaaToolkitAdbDeviceGetScreencapMethods(device_ptr);
                let input_methods = sys::MaaToolkitAdbDeviceGetInputMethods(device_ptr);

                let config_str =
                    CStr::from_ptr(sys::MaaToolkitAdbDeviceGetConfig(device_ptr)).to_string_lossy();
                let config = serde_json::from_str(&config_str).unwrap_or(serde_json::Value::Null);

                devices.push(AdbDevice {
                    name,
                    adb_path: PathBuf::from(adb_path_str),
                    address,
                    screencap_methods,
                    input_methods,
                    config,
                });
            }
            Ok(devices)
        }
    }

    pub fn find_desktop_windows() -> MaaResult<Vec<DesktopWindow>> {
        let list = unsafe { sys::MaaToolkitDesktopWindowListCreate() };
        if list.is_null() {
            return Err(MaaError::NullPointer);
        }

        let _guard = DesktopWindowListGuard(list);

        unsafe {
            let ret = sys::MaaToolkitDesktopWindowFindAll(list);
            common::check_bool(ret)?;

            let count = sys::MaaToolkitDesktopWindowListSize(list);
            let mut windows = Vec::with_capacity(count as usize);

            for i in 0..count {
                let win_ptr = sys::MaaToolkitDesktopWindowListAt(list, i);
                if win_ptr.is_null() {
                    continue;
                }

                let hwnd = sys::MaaToolkitDesktopWindowGetHandle(win_ptr);

                let class_name = CStr::from_ptr(sys::MaaToolkitDesktopWindowGetClassName(win_ptr))
                    .to_string_lossy()
                    .into_owned();

                let window_name =
                    CStr::from_ptr(sys::MaaToolkitDesktopWindowGetWindowName(win_ptr))
                        .to_string_lossy()
                        .into_owned();

                windows.push(DesktopWindow {
                    hwnd,
                    class_name,
                    window_name,
                });
            }
            Ok(windows)
        }
    }
}

struct AdbDeviceListGuard(*mut sys::MaaToolkitAdbDeviceList);
impl Drop for AdbDeviceListGuard {
    fn drop(&mut self) {
        unsafe { sys::MaaToolkitAdbDeviceListDestroy(self.0) }
    }
}

struct DesktopWindowListGuard(*mut sys::MaaToolkitDesktopWindowList);
impl Drop for DesktopWindowListGuard {
    fn drop(&mut self) {
        unsafe { sys::MaaToolkitDesktopWindowListDestroy(self.0) }
    }
}
