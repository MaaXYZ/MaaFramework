use crate::buffer::{ImageBuffer, StringBuffer};
use crate::define::*;
use crate::error::{MaaError, Result};
use crate::ffi;
use crate::job::CtrlJob;
use serde_json::Value;
use std::ffi::CString;
use std::path::Path;

/// Controller manages device input/output operations.
///
/// # Thread Safety
/// The underlying MaaController handle is designed to be thread-safe by the C API.
/// Multiple threads can submit operations, but the operations are queued and
/// executed sequentially by the controller.
pub struct Controller {
    handle: MaaControllerHandle,
    own: bool,
}

impl Controller {
    pub(crate) fn from_handle(handle: MaaControllerHandle) -> Self {
        Self { handle, own: false }
    }

    pub fn handle(&self) -> MaaControllerHandle {
        self.handle
    }

    pub fn post_connection(&self) -> Result<CtrlJob> {
        let id = ffi::maa_controller_post_connection(self.handle);
        if id == MAA_INVALID_ID {
            return Err(MaaError::OperationFailed("post_connection"));
        }
        Ok(CtrlJob::new(id, self.handle))
    }

    pub fn post_click(&self, x: i32, y: i32) -> Result<CtrlJob> {
        let id = ffi::maa_controller_post_click(self.handle, x, y);
        if id == MAA_INVALID_ID {
            return Err(MaaError::OperationFailed("post_click"));
        }
        Ok(CtrlJob::new(id, self.handle))
    }

    pub fn post_swipe(&self, x1: i32, y1: i32, x2: i32, y2: i32, duration: i32) -> Result<CtrlJob> {
        let id = ffi::maa_controller_post_swipe(self.handle, x1, y1, x2, y2, duration);
        if id == MAA_INVALID_ID {
            return Err(MaaError::OperationFailed("post_swipe"));
        }
        Ok(CtrlJob::new(id, self.handle))
    }

    pub fn post_click_key(&self, key: i32) -> Result<CtrlJob> {
        let id = ffi::maa_controller_post_click_key(self.handle, key);
        if id == MAA_INVALID_ID {
            return Err(MaaError::OperationFailed("post_click_key"));
        }
        Ok(CtrlJob::new(id, self.handle))
    }

    pub fn post_key_down(&self, key: i32) -> Result<CtrlJob> {
        let id = ffi::maa_controller_post_key_down(self.handle, key);
        if id == MAA_INVALID_ID {
            return Err(MaaError::OperationFailed("post_key_down"));
        }
        Ok(CtrlJob::new(id, self.handle))
    }

    pub fn post_key_up(&self, key: i32) -> Result<CtrlJob> {
        let id = ffi::maa_controller_post_key_up(self.handle, key);
        if id == MAA_INVALID_ID {
            return Err(MaaError::OperationFailed("post_key_up"));
        }
        Ok(CtrlJob::new(id, self.handle))
    }

    pub fn post_input_text(&self, text: &str) -> Result<CtrlJob> {
        let cstr = CString::new(text).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let id = ffi::maa_controller_post_input_text(self.handle, cstr.as_ptr());
        if id == MAA_INVALID_ID {
            return Err(MaaError::OperationFailed("post_input_text"));
        }
        Ok(CtrlJob::new(id, self.handle))
    }

    pub fn post_start_app(&self, intent: &str) -> Result<CtrlJob> {
        let cstr = CString::new(intent).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let id = ffi::maa_controller_post_start_app(self.handle, cstr.as_ptr());
        if id == MAA_INVALID_ID {
            return Err(MaaError::OperationFailed("post_start_app"));
        }
        Ok(CtrlJob::new(id, self.handle))
    }

    pub fn post_stop_app(&self, intent: &str) -> Result<CtrlJob> {
        let cstr = CString::new(intent).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let id = ffi::maa_controller_post_stop_app(self.handle, cstr.as_ptr());
        if id == MAA_INVALID_ID {
            return Err(MaaError::OperationFailed("post_stop_app"));
        }
        Ok(CtrlJob::new(id, self.handle))
    }

    pub fn post_touch_down(&self, contact: i32, x: i32, y: i32, pressure: i32) -> Result<CtrlJob> {
        let id = ffi::maa_controller_post_touch_down(self.handle, contact, x, y, pressure);
        if id == MAA_INVALID_ID {
            return Err(MaaError::OperationFailed("post_touch_down"));
        }
        Ok(CtrlJob::new(id, self.handle))
    }

    pub fn post_touch_move(&self, contact: i32, x: i32, y: i32, pressure: i32) -> Result<CtrlJob> {
        let id = ffi::maa_controller_post_touch_move(self.handle, contact, x, y, pressure);
        if id == MAA_INVALID_ID {
            return Err(MaaError::OperationFailed("post_touch_move"));
        }
        Ok(CtrlJob::new(id, self.handle))
    }

    pub fn post_touch_up(&self, contact: i32) -> Result<CtrlJob> {
        let id = ffi::maa_controller_post_touch_up(self.handle, contact);
        if id == MAA_INVALID_ID {
            return Err(MaaError::OperationFailed("post_touch_up"));
        }
        Ok(CtrlJob::new(id, self.handle))
    }

    pub fn post_screencap(&self) -> Result<CtrlJob> {
        let id = ffi::maa_controller_post_screencap(self.handle);
        if id == MAA_INVALID_ID {
            return Err(MaaError::OperationFailed("post_screencap"));
        }
        Ok(CtrlJob::new(id, self.handle))
    }

    pub fn connected(&self) -> bool {
        ffi::maa_controller_connected(self.handle) != 0
    }

    pub fn cached_image(&self) -> Result<ImageBuffer> {
        let buffer = ImageBuffer::new();
        if ffi::maa_controller_cached_image(self.handle, buffer.handle()) == 0 {
            return Err(MaaError::OperationFailed("get cached image"));
        }
        Ok(buffer)
    }

    pub fn uuid(&self) -> Result<String> {
        let buffer = StringBuffer::new();
        if ffi::maa_controller_get_uuid(self.handle, buffer.handle()) == 0 {
            return Err(MaaError::OperationFailed("get uuid"));
        }
        buffer.get()
    }

    pub fn set_screenshot_target_long_side(&self, long_side: i32) -> bool {
        ffi::maa_controller_set_option(
            self.handle,
            CtrlOption::ScreenshotTargetLongSide as i32,
            &long_side as *const i32 as *mut std::ffi::c_void,
            std::mem::size_of::<i32>() as u64,
        ) != 0
    }

    pub fn set_screenshot_target_short_side(&self, short_side: i32) -> bool {
        ffi::maa_controller_set_option(
            self.handle,
            CtrlOption::ScreenshotTargetShortSide as i32,
            &short_side as *const i32 as *mut std::ffi::c_void,
            std::mem::size_of::<i32>() as u64,
        ) != 0
    }

    pub fn set_screenshot_use_raw_size(&self, enable: bool) -> bool {
        let val: u8 = if enable { 1 } else { 0 };
        ffi::maa_controller_set_option(
            self.handle,
            CtrlOption::ScreenshotUseRawSize as i32,
            &val as *const u8 as *mut std::ffi::c_void,
            std::mem::size_of::<u8>() as u64,
        ) != 0
    }
}

impl Drop for Controller {
    fn drop(&mut self) {
        if self.own && !self.handle.is_null() {
            ffi::maa_controller_destroy(self.handle);
        }
    }
}

// SAFETY: MaaController is documented as thread-safe by the MaaFramework C API.
// The underlying handle can be safely shared across threads.
// Operations are queued and executed sequentially by the controller.
unsafe impl Send for Controller {}
unsafe impl Sync for Controller {}

pub struct AdbController {
    inner: Controller,
}

impl AdbController {
    pub fn new(
        adb_path: impl AsRef<Path>,
        address: &str,
        screencap_methods: MaaAdbScreencapMethod,
        input_methods: MaaAdbInputMethod,
        config: &Value,
        agent_path: impl AsRef<Path>,
    ) -> Result<Self> {
        let adb_path_str = adb_path.as_ref().to_string_lossy();
        let adb_cstr =
            CString::new(adb_path_str.as_ref()).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let address_cstr =
            CString::new(address).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let config_str = serde_json::to_string(config)?;
        let config_cstr =
            CString::new(config_str).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let agent_path_str = agent_path.as_ref().to_string_lossy();
        let agent_cstr = CString::new(agent_path_str.as_ref())
            .map_err(|e| MaaError::BufferError(e.to_string()))?;

        let handle = ffi::maa_adb_controller_create(
            adb_cstr.as_ptr(),
            address_cstr.as_ptr(),
            screencap_methods,
            input_methods,
            config_cstr.as_ptr(),
            agent_cstr.as_ptr(),
        );

        if handle.is_null() {
            return Err(MaaError::CreateFailed("AdbController"));
        }

        Ok(Self {
            inner: Controller {
                handle,
                own: true,
            },
        })
    }
}

impl std::ops::Deref for AdbController {
    type Target = Controller;
    fn deref(&self) -> &Self::Target {
        &self.inner
    }
}

impl std::ops::DerefMut for AdbController {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.inner
    }
}

pub struct Win32Controller {
    inner: Controller,
}

impl Win32Controller {
    pub fn new(
        hwnd: *mut std::ffi::c_void,
        screencap_method: MaaWin32ScreencapMethod,
        mouse_method: MaaWin32InputMethod,
        keyboard_method: MaaWin32InputMethod,
    ) -> Result<Self> {
        let handle = ffi::maa_win32_controller_create(
            hwnd,
            screencap_method,
            mouse_method,
            keyboard_method,
        );

        if handle.is_null() {
            return Err(MaaError::CreateFailed("Win32Controller"));
        }

        Ok(Self {
            inner: Controller {
                handle,
                own: true,
            },
        })
    }
}

impl std::ops::Deref for Win32Controller {
    type Target = Controller;
    fn deref(&self) -> &Self::Target {
        &self.inner
    }
}

impl std::ops::DerefMut for Win32Controller {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.inner
    }
}

pub struct DbgController {
    inner: Controller,
}

impl DbgController {
    pub fn new(
        read_path: impl AsRef<Path>,
        write_path: impl AsRef<Path>,
        dbg_type: MaaDbgControllerType,
        config: &Value,
    ) -> Result<Self> {
        let read_path_str = read_path.as_ref().to_string_lossy();
        let read_cstr = CString::new(read_path_str.as_ref())
            .map_err(|e| MaaError::BufferError(e.to_string()))?;
        let write_path_str = write_path.as_ref().to_string_lossy();
        let write_cstr = CString::new(write_path_str.as_ref())
            .map_err(|e| MaaError::BufferError(e.to_string()))?;
        let config_str = serde_json::to_string(config)?;
        let config_cstr =
            CString::new(config_str).map_err(|e| MaaError::BufferError(e.to_string()))?;

        let handle = ffi::maa_dbg_controller_create(
            read_cstr.as_ptr(),
            write_cstr.as_ptr(),
            dbg_type,
            config_cstr.as_ptr(),
        );

        if handle.is_null() {
            return Err(MaaError::CreateFailed("DbgController"));
        }

        Ok(Self {
            inner: Controller {
                handle,
                own: true,
            },
        })
    }
}

impl std::ops::Deref for DbgController {
    type Target = Controller;
    fn deref(&self) -> &Self::Target {
        &self.inner
    }
}

impl std::ops::DerefMut for DbgController {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.inner
    }
}
