use crate::{common, sys, MaaError, MaaResult};
use std::ffi::CString;
use std::os::raw::c_void;
use std::ptr::NonNull;

/// Device controller interface.
///
/// Handles interaction with the target device, including:
/// - Input events (click, swipe, key press)
/// - Screen capture
/// - App management (start/stop)
/// - Connection management
pub struct Controller {
    handle: NonNull<sys::MaaController>,
}

unsafe impl Send for Controller {}
unsafe impl Sync for Controller {}

impl Controller {
    /// Create a new ADB controller for Android device control.
    ///
    /// # Arguments
    /// * `adb_path` - Path to the ADB executable
    /// * `address` - Device address (e.g., "127.0.0.1:5555" or "emulator-5554")
    /// * `config` - JSON configuration string for advanced options
    /// * `agent_path` - Optional path to MaaAgentBinary
    ///
    /// # Example
    /// ```ignore
    /// let controller = Controller::new_adb(
    ///     "adb",
    ///     "127.0.0.1:5555",
    ///     "{}",
    ///     None
    /// )?;
    /// ```
    #[cfg(feature = "adb")]
    pub fn new_adb(
        adb_path: &str,
        address: &str,
        config: &str,
        agent_path: Option<&str>,
    ) -> MaaResult<Self> {
        let c_adb = CString::new(adb_path)?;
        let c_addr = CString::new(address)?;
        let c_cfg = CString::new(config)?;
        let c_agent = agent_path.map(CString::new).transpose()?;
        let agent_ptr = c_agent
            .as_ref()
            .map(|c| c.as_ptr())
            .unwrap_or(std::ptr::null());

        let handle = unsafe {
            sys::MaaAdbControllerCreate(
                c_adb.as_ptr(),
                c_addr.as_ptr(),
                sys::MaaAdbScreencapMethod_Default as sys::MaaAdbScreencapMethod,
                sys::MaaAdbInputMethod_Default as sys::MaaAdbInputMethod,
                c_cfg.as_ptr(),
                agent_ptr,
            )
        };

        if let Some(ptr) = NonNull::new(handle) {
            Ok(Self { handle: ptr })
        } else {
            Err(MaaError::FrameworkError(-1))
        }
    }

    /// Create a new Win32 controller for Windows window control.
    ///
    /// # Arguments
    /// * `hwnd` - Window handle (HWND)
    /// * `screencap_method` - Screenshot capture method
    /// * `mouse_method` - Mouse input method
    /// * `keyboard_method` - Keyboard input method
    #[cfg(feature = "win32")]
    pub fn new_win32(
        hwnd: *mut c_void,
        screencap_method: sys::MaaWin32ScreencapMethod,
        mouse_method: sys::MaaWin32InputMethod,
        keyboard_method: sys::MaaWin32InputMethod,
    ) -> MaaResult<Self> {
        let handle = unsafe {
            sys::MaaWin32ControllerCreate(hwnd, screencap_method, mouse_method, keyboard_method)
        };

        if let Some(ptr) = NonNull::new(handle) {
            Ok(Self { handle: ptr })
        } else {
            Err(MaaError::FrameworkError(-1))
        }
    }

    /// Create a new PlayCover controller for iOS app control on macOS.
    ///
    /// # Arguments
    /// * `address` - PlayCover connection address
    /// * `uuid` - Device UUID
    pub fn new_playcover(address: &str, uuid: &str) -> MaaResult<Self> {
        let c_addr = CString::new(address)?;
        let c_uuid = CString::new(uuid)?;
        let handle = unsafe { sys::MaaPlayCoverControllerCreate(c_addr.as_ptr(), c_uuid.as_ptr()) };

        if let Some(ptr) = NonNull::new(handle) {
            Ok(Self { handle: ptr })
        } else {
            Err(MaaError::FrameworkError(-1))
        }
    }

    /// Create a custom controller with user-defined callbacks.
    ///
    /// # Arguments
    /// * `callback` - Implementation of `CustomControllerCallback` trait
    #[cfg(feature = "custom")]
    pub fn new_custom<T: crate::custom_controller::CustomControllerCallback + 'static>(
        callback: T,
    ) -> MaaResult<Self> {
        let boxed: Box<Box<dyn crate::custom_controller::CustomControllerCallback>> =
            Box::new(Box::new(callback));
        let cb_ptr = Box::into_raw(boxed) as *mut c_void;
        let callbacks = crate::custom_controller::get_callbacks();
        let handle =
            unsafe { sys::MaaCustomControllerCreate(callbacks as *const _ as *mut _, cb_ptr) };
        NonNull::new(handle)
            .map(|ptr| Self { handle: ptr })
            .ok_or_else(|| {
                unsafe {
                    let _ = Box::from_raw(
                        cb_ptr as *mut Box<dyn crate::custom_controller::CustomControllerCallback>,
                    );
                }
                MaaError::FrameworkError(-1)
            })
    }

    /// Post a click action at the specified coordinates.
    ///
    /// Returns immediately with an operation ID. Use `wait()` to block until complete.
    pub fn post_click(&self, x: i32, y: i32) -> MaaResult<common::MaaId> {
        let id = unsafe { sys::MaaControllerPostClick(self.handle.as_ptr(), x, y) };
        Ok(id)
    }

    /// Post a screenshot capture request.
    ///
    /// Returns immediately. After completion, use `cached_image()` to retrieve the result.
    pub fn post_screencap(&self) -> MaaResult<common::MaaId> {
        let id = unsafe { sys::MaaControllerPostScreencap(self.handle.as_ptr()) };
        Ok(id)
    }

    pub fn post_click_v2(
        &self,
        x: i32,
        y: i32,
        contact: i32,
        pressure: i32,
    ) -> MaaResult<common::MaaId> {
        let id =
            unsafe { sys::MaaControllerPostClickV2(self.handle.as_ptr(), x, y, contact, pressure) };
        Ok(id)
    }

    pub fn post_swipe(
        &self,
        x1: i32,
        y1: i32,
        x2: i32,
        y2: i32,
        duration: i32,
    ) -> MaaResult<common::MaaId> {
        let id =
            unsafe { sys::MaaControllerPostSwipe(self.handle.as_ptr(), x1, y1, x2, y2, duration) };
        Ok(id)
    }

    pub fn post_click_key(&self, keycode: i32) -> MaaResult<common::MaaId> {
        let id = unsafe { sys::MaaControllerPostClickKey(self.handle.as_ptr(), keycode) };
        Ok(id)
    }

    pub fn post_press(&self, keycode: i32) -> MaaResult<common::MaaId> {
        self.post_click_key(keycode)
    }

    pub fn post_input_text(&self, text: &str) -> MaaResult<common::MaaId> {
        let c_text = CString::new(text)?;
        let id = unsafe { sys::MaaControllerPostInputText(self.handle.as_ptr(), c_text.as_ptr()) };
        Ok(id)
    }

    pub fn post_shell(&self, cmd: &str, timeout: i64) -> MaaResult<common::MaaId> {
        let c_cmd = CString::new(cmd)?;
        let id =
            unsafe { sys::MaaControllerPostShell(self.handle.as_ptr(), c_cmd.as_ptr(), timeout) };
        Ok(id)
    }

    pub fn post_touch_down(
        &self,
        contact: i32,
        x: i32,
        y: i32,
        pressure: i32,
    ) -> MaaResult<common::MaaId> {
        let id = unsafe {
            sys::MaaControllerPostTouchDown(self.handle.as_ptr(), contact, x, y, pressure)
        };
        Ok(id)
    }

    pub fn post_touch_move(
        &self,
        contact: i32,
        x: i32,
        y: i32,
        pressure: i32,
    ) -> MaaResult<common::MaaId> {
        let id = unsafe {
            sys::MaaControllerPostTouchMove(self.handle.as_ptr(), contact, x, y, pressure)
        };
        Ok(id)
    }

    pub fn post_touch_up(&self, contact: i32) -> MaaResult<common::MaaId> {
        let id = unsafe { sys::MaaControllerPostTouchUp(self.handle.as_ptr(), contact) };
        Ok(id)
    }

    pub fn raw(&self) -> *mut sys::MaaController {
        self.handle.as_ptr()
    }

    // === Connection ===

    pub fn post_connection(&self) -> MaaResult<common::MaaId> {
        let id = unsafe { sys::MaaControllerPostConnection(self.handle.as_ptr()) };
        Ok(id)
    }

    pub fn connected(&self) -> bool {
        unsafe { sys::MaaControllerConnected(self.handle.as_ptr()) != 0 }
    }

    pub fn uuid(&self) -> MaaResult<String> {
        let buffer = crate::buffer::MaaStringBuffer::new()?;
        let ret = unsafe { sys::MaaControllerGetUuid(self.handle.as_ptr(), buffer.raw()) };
        if ret != 0 {
            Ok(buffer.to_string())
        } else {
            Err(MaaError::FrameworkError(0))
        }
    }

    pub fn resolution(&self) -> MaaResult<(i32, i32)> {
        let mut width: i32 = 0;
        let mut height: i32 = 0;
        let ret = unsafe {
            sys::MaaControllerGetResolution(self.handle.as_ptr(), &mut width, &mut height)
        };
        if ret != 0 {
            Ok((width, height))
        } else {
            Err(MaaError::FrameworkError(0))
        }
    }

    // === Swipe V2 ===

    pub fn post_swipe_v2(
        &self,
        x1: i32,
        y1: i32,
        x2: i32,
        y2: i32,
        duration: i32,
        contact: i32,
        pressure: i32,
    ) -> MaaResult<common::MaaId> {
        let id = unsafe {
            sys::MaaControllerPostSwipeV2(
                self.handle.as_ptr(),
                x1,
                y1,
                x2,
                y2,
                duration,
                contact,
                pressure,
            )
        };
        Ok(id)
    }

    // === Key control ===

    pub fn post_key_down(&self, keycode: i32) -> MaaResult<common::MaaId> {
        let id = unsafe { sys::MaaControllerPostKeyDown(self.handle.as_ptr(), keycode) };
        Ok(id)
    }

    pub fn post_key_up(&self, keycode: i32) -> MaaResult<common::MaaId> {
        let id = unsafe { sys::MaaControllerPostKeyUp(self.handle.as_ptr(), keycode) };
        Ok(id)
    }

    // === App control ===

    pub fn post_start_app(&self, intent: &str) -> MaaResult<common::MaaId> {
        let c_intent = CString::new(intent)?;
        let id = unsafe { sys::MaaControllerPostStartApp(self.handle.as_ptr(), c_intent.as_ptr()) };
        Ok(id)
    }

    pub fn post_stop_app(&self, intent: &str) -> MaaResult<common::MaaId> {
        let c_intent = CString::new(intent)?;
        let id = unsafe { sys::MaaControllerPostStopApp(self.handle.as_ptr(), c_intent.as_ptr()) };
        Ok(id)
    }

    // === Scroll ===

    pub fn post_scroll(&self, dx: i32, dy: i32) -> MaaResult<common::MaaId> {
        let id = unsafe { sys::MaaControllerPostScroll(self.handle.as_ptr(), dx, dy) };
        Ok(id)
    }

    // === Image ===

    pub fn cached_image(&self) -> MaaResult<crate::buffer::MaaImageBuffer> {
        let buffer = crate::buffer::MaaImageBuffer::new()?;
        let ret = unsafe { sys::MaaControllerCachedImage(self.handle.as_ptr(), buffer.raw()) };
        if ret != 0 {
            Ok(buffer)
        } else {
            Err(MaaError::FrameworkError(0))
        }
    }

    // === Shell output ===

    pub fn shell_output(&self) -> MaaResult<String> {
        let buffer = crate::buffer::MaaStringBuffer::new()?;
        let ret = unsafe { sys::MaaControllerGetShellOutput(self.handle.as_ptr(), buffer.raw()) };
        if ret != 0 {
            Ok(buffer.to_string())
        } else {
            Err(MaaError::FrameworkError(0))
        }
    }

    // === Status ===

    pub fn status(&self, ctrl_id: common::MaaId) -> common::MaaStatus {
        let s = unsafe { sys::MaaControllerStatus(self.handle.as_ptr(), ctrl_id) };
        common::MaaStatus(s)
    }

    pub fn wait(&self, ctrl_id: common::MaaId) -> common::MaaStatus {
        let s = unsafe { sys::MaaControllerWait(self.handle.as_ptr(), ctrl_id) };
        common::MaaStatus(s)
    }

    // === Screenshot options ===

    pub fn set_screenshot_target_long_side(&self, long_side: i32) -> MaaResult<()> {
        let mut val = long_side;
        let ret = unsafe {
            sys::MaaControllerSetOption(
                self.handle.as_ptr(),
                sys::MaaCtrlOptionEnum_MaaCtrlOption_ScreenshotTargetLongSide as i32,
                &mut val as *mut _ as *mut c_void,
                std::mem::size_of::<i32>() as u64,
            )
        };
        common::check_bool(ret)
    }

    pub fn set_screenshot_target_short_side(&self, short_side: i32) -> MaaResult<()> {
        let mut val = short_side;
        let ret = unsafe {
            sys::MaaControllerSetOption(
                self.handle.as_ptr(),
                sys::MaaCtrlOptionEnum_MaaCtrlOption_ScreenshotTargetShortSide as i32,
                &mut val as *mut _ as *mut c_void,
                std::mem::size_of::<i32>() as u64,
            )
        };
        common::check_bool(ret)
    }

    pub fn set_screenshot_use_raw_size(&self, enable: bool) -> MaaResult<()> {
        let mut val: u8 = if enable { 1 } else { 0 };
        let ret = unsafe {
            sys::MaaControllerSetOption(
                self.handle.as_ptr(),
                sys::MaaCtrlOptionEnum_MaaCtrlOption_ScreenshotUseRawSize as i32,
                &mut val as *mut _ as *mut c_void,
                std::mem::size_of::<u8>() as u64,
            )
        };
        common::check_bool(ret)
    }

    // === New controller types ===

    #[cfg(feature = "dbg")]
    pub fn new_dbg(
        read_path: &str,
        write_path: &str,
        dbg_type: sys::MaaDbgControllerType,
        config: &str,
    ) -> MaaResult<Self> {
        let c_read = CString::new(read_path)?;
        let c_write = CString::new(write_path)?;
        let c_cfg = CString::new(config)?;
        let handle = unsafe {
            sys::MaaDbgControllerCreate(c_read.as_ptr(), c_write.as_ptr(), dbg_type, c_cfg.as_ptr())
        };
        NonNull::new(handle)
            .map(|ptr| Self { handle: ptr })
            .ok_or(MaaError::FrameworkError(-1))
    }

    /// Create a virtual gamepad controller (Windows only).
    ///
    /// Emulates Xbox 360 or DualShock 4 gamepad via ViGEm for controlling
    /// games that require gamepad input.
    ///
    /// Requires ViGEm Bus Driver: <https://github.com/ViGEm/ViGEmBus/releases>
    ///
    /// # Gamepad Operation Mapping
    ///
    /// - `post_click_key`/`post_key_down`/`post_key_up`: Digital buttons (use `GamepadButton` values)
    /// - `post_touch_down`/`post_touch_move`/`post_touch_up`: Analog sticks and triggers
    ///   - `GamepadContact::LeftStick`: Left stick (x, y: -32768~32767)
    ///   - `GamepadContact::RightStick`: Right stick (x, y: -32768~32767)
    ///   - `GamepadContact::LeftTrigger`: Left trigger (pressure: 0~255)
    ///   - `GamepadContact::RightTrigger`: Right trigger (pressure: 0~255)
    ///
    /// # Arguments
    /// * `hwnd` - Window handle for screencap (can be null if screencap not needed)
    /// * `gamepad_type` - Gamepad type (`GamepadType::Xbox360` or `GamepadType::DualShock4`)
    /// * `screencap_method` - Screenshot method for the window
    ///
    /// # Example
    /// ```ignore
    /// use maa_framework::common::{GamepadType, Win32ScreencapMethod};
    /// let controller = Controller::new_gamepad(
    ///     hwnd,
    ///     GamepadType::Xbox360,
    ///     Win32ScreencapMethod::FRAME_POOL,
    /// )?;
    /// ```
    #[cfg(feature = "win32")]
    pub fn new_gamepad(
        hwnd: *mut c_void,
        gamepad_type: crate::common::GamepadType,
        screencap_method: crate::common::Win32ScreencapMethod,
    ) -> MaaResult<Self> {
        let handle = unsafe {
            sys::MaaGamepadControllerCreate(hwnd, gamepad_type as u64, screencap_method.bits())
        };
        NonNull::new(handle)
            .map(|ptr| Self { handle: ptr })
            .ok_or(MaaError::FrameworkError(-1))
    }

    // === EventSink ===

    /// Returns sink_id for later removal. Callback lifetime managed by caller.
    pub fn add_sink<F>(&self, callback: F) -> MaaResult<sys::MaaSinkId>
    where
        F: Fn(&str, &str) + Send + Sync + 'static,
    {
        let (cb_fn, cb_arg) = crate::callback::EventCallback::new(callback);
        let sink_id = unsafe { sys::MaaControllerAddSink(self.handle.as_ptr(), cb_fn, cb_arg) };
        if sink_id != 0 {
            Ok(sink_id)
        } else {
            unsafe { crate::callback::EventCallback::drop_callback(cb_arg) };
            Err(MaaError::FrameworkError(0))
        }
    }

    pub fn remove_sink(&self, sink_id: sys::MaaSinkId) {
        unsafe { sys::MaaControllerRemoveSink(self.handle.as_ptr(), sink_id) }
    }

    pub fn clear_sinks(&self) {
        unsafe { sys::MaaControllerClearSinks(self.handle.as_ptr()) }
    }
}

impl Drop for Controller {
    fn drop(&mut self) {
        unsafe {
            sys::MaaControllerClearSinks(self.handle.as_ptr());
            sys::MaaControllerDestroy(self.handle.as_ptr());
        }
    }
}

/// Builder for ADB controller configuration.
///
/// Provides a fluent API for configuring ADB controllers with sensible defaults.
///
/// # Example
/// ```ignore
/// let controller = AdbControllerBuilder::new("adb", "127.0.0.1:5555")
///     .screencap_methods(MaaAdbScreencapMethod_Default)
///     .input_methods(MaaAdbInputMethod_Default)
///     .config(r#"{"adb_delay": 100}"#)
///     .agent_path("/path/to/agent")
///     .build()?;
/// ```
#[cfg(feature = "adb")]
pub struct AdbControllerBuilder {
    adb_path: String,
    address: String,
    screencap_methods: sys::MaaAdbScreencapMethod,
    input_methods: sys::MaaAdbInputMethod,
    config: String,
    agent_path: Option<String>,
}

#[cfg(feature = "adb")]
impl AdbControllerBuilder {
    /// Create a new builder with required ADB path and device address.
    pub fn new(adb_path: &str, address: &str) -> Self {
        Self {
            adb_path: adb_path.to_string(),
            address: address.to_string(),
            screencap_methods: sys::MaaAdbScreencapMethod_Default as sys::MaaAdbScreencapMethod,
            input_methods: sys::MaaAdbInputMethod_Default as sys::MaaAdbInputMethod,
            config: "{}".to_string(),
            agent_path: None,
        }
    }

    /// Set the screencap methods to use.
    ///
    /// Default: `MaaAdbScreencapMethod_Default`
    pub fn screencap_methods(mut self, methods: sys::MaaAdbScreencapMethod) -> Self {
        self.screencap_methods = methods;
        self
    }

    /// Set the input methods to use.
    ///
    /// Default: `MaaAdbInputMethod_Default`
    pub fn input_methods(mut self, methods: sys::MaaAdbInputMethod) -> Self {
        self.input_methods = methods;
        self
    }

    /// Set additional configuration as JSON.
    ///
    /// Default: `"{}"`
    pub fn config(mut self, config: &str) -> Self {
        self.config = config.to_string();
        self
    }

    /// Set the path to MaaAgentBinary.
    ///
    /// If not set, the framework will use the default agent path.
    pub fn agent_path(mut self, path: &str) -> Self {
        self.agent_path = Some(path.to_string());
        self
    }

    /// Build the controller with the configured options.
    pub fn build(self) -> MaaResult<Controller> {
        Controller::new_adb(
            &self.adb_path,
            &self.address,
            &self.config,
            self.agent_path.as_deref(),
        )
    }
}

/// A borrowed reference to a Controller.
///
/// This is a non-owning view that can be used for read-only operations.
/// It does NOT call destroy when dropped and should only be used while
/// the underlying Controller is still alive.
///
/// Obtained from `Tasker::controller()`.
pub struct ControllerRef<'a> {
    handle: *mut sys::MaaController,
    _marker: std::marker::PhantomData<&'a ()>,
}

impl<'a> ControllerRef<'a> {
    pub(crate) fn from_ptr(handle: *mut sys::MaaController) -> Option<Self> {
        if handle.is_null() {
            None
        } else {
            Some(Self {
                handle,
                _marker: std::marker::PhantomData,
            })
        }
    }

    /// Check if connected.
    pub fn connected(&self) -> bool {
        unsafe { sys::MaaControllerConnected(self.handle) != 0 }
    }

    /// Get device UUID.
    pub fn uuid(&self) -> MaaResult<String> {
        let buffer = crate::buffer::MaaStringBuffer::new()?;
        let ret = unsafe { sys::MaaControllerGetUuid(self.handle, buffer.raw()) };
        if ret != 0 {
            Ok(buffer.to_string())
        } else {
            Err(MaaError::FrameworkError(0))
        }
    }

    /// Get device resolution.
    pub fn resolution(&self) -> MaaResult<(i32, i32)> {
        let mut width: i32 = 0;
        let mut height: i32 = 0;
        let ret = unsafe { sys::MaaControllerGetResolution(self.handle, &mut width, &mut height) };
        if ret != 0 {
            Ok((width, height))
        } else {
            Err(MaaError::FrameworkError(0))
        }
    }

    /// Get operation status.
    pub fn status(&self, ctrl_id: common::MaaId) -> common::MaaStatus {
        let s = unsafe { sys::MaaControllerStatus(self.handle, ctrl_id) };
        common::MaaStatus(s)
    }

    /// Wait for operation to complete.
    pub fn wait(&self, ctrl_id: common::MaaId) -> common::MaaStatus {
        let s = unsafe { sys::MaaControllerWait(self.handle, ctrl_id) };
        common::MaaStatus(s)
    }

    /// Get cached screenshot.
    pub fn cached_image(&self) -> MaaResult<crate::buffer::MaaImageBuffer> {
        let buffer = crate::buffer::MaaImageBuffer::new()?;
        let ret = unsafe { sys::MaaControllerCachedImage(self.handle, buffer.raw()) };
        if ret != 0 {
            Ok(buffer)
        } else {
            Err(MaaError::FrameworkError(0))
        }
    }

    /// Get raw handle.
    pub fn raw(&self) -> *mut sys::MaaController {
        self.handle
    }
}
