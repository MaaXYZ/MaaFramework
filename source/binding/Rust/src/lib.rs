//! # MaaFramework Rust Bindings
//!
//! High-performance, safe Rust bindings for [MaaFramework](https://github.com/MaaAssistantArknights/MaaFramework),
//! a game automation framework based on image recognition.
//!
//! ## Quick Start
//!
//! ```ignore
//! use maa_framework::{Tasker, Resource, Controller, Toolkit};
//!
//! // 1. Find devices
//! let devices = Toolkit::find_adb_devices()?;
//!
//! // 2. Create components
//! let tasker = Tasker::new()?;
//! let resource = Resource::new()?;
//! let controller = Controller::new_adb(&adb_path, &address, "{}", None)?;
//!
//! // 3. Bind and run
//! tasker.bind(&resource, &controller)?;
//! let job = tasker.post_task("StartTask", "{}")?;
//! let result = job.get(true)?;
//! ```
//!
//! ## Core Modules
//!
//! | Module | Description |
//! |--------|-------------|
//! | [`tasker`] | Task execution and pipeline management |
//! | [`resource`] | Resource loading (images, models, pipelines) |
//! | [`controller`] | Device control (ADB, Win32, PlayCover) |
//! | [`context`] | Task execution context for custom components |
//! | [`toolkit`] | Device discovery utilities |
//! | [`buffer`] | Safe data buffers for FFI |
//! | [`custom`] | Custom recognizer and action traits |
//! | [`notification`] | Structured event notification parsing |
//! | [`agent_client`] | Remote custom component client |
//! | [`agent_server`] | Remote custom component server |
//!
//! ## Feature Flags
//!
//! - `adb` - ADB controller support (default)
//! - `win32` - Win32 controller support (Windows only)
//! - `custom` - Custom recognizer/action/controller support
//! - `toolkit` - Device discovery utilities
//! - `image` - Integration with the `image` crate

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

pub mod agent_client;
pub mod agent_server;
pub mod buffer;
pub mod callback;
pub mod common;
pub mod context;
pub mod controller;
pub mod custom;
pub mod custom_controller;
pub mod error;
pub mod event_sink;
pub mod job;
pub mod notification;
pub mod pipeline;
pub mod resource;
pub mod sys;
pub mod tasker;
pub mod toolkit;
pub mod util;

pub use common::MaaStatus;
pub use error::{MaaError, MaaResult};

use std::ffi::CString;

/// Get the MaaFramework version string.
///
/// # Example
/// ```ignore
/// println!("MaaFramework version: {}", maa_framework::maa_version());
/// ```
pub fn maa_version() -> &'static str {
    unsafe {
        std::ffi::CStr::from_ptr(sys::MaaVersion())
            .to_str()
            .unwrap_or("unknown")
    }
}

/// Set a global framework option.
///
/// Low-level function for setting global options. Consider using the
/// convenience wrappers like [`configure_logging`], [`set_debug_mode`], etc.
pub fn set_global_option(
    key: sys::MaaGlobalOption,
    value: *mut std::ffi::c_void,
    size: u64,
) -> MaaResult<()> {
    let ret = unsafe { sys::MaaGlobalSetOption(key, value, size) };
    common::check_bool(ret)
}

/// Configure the log output directory.
///
/// # Arguments
/// * `log_dir` - Path to the directory where logs should be stored
pub fn configure_logging(log_dir: &str) -> MaaResult<()> {
    let c_dir = CString::new(log_dir)?;
    set_global_option(
        sys::MaaGlobalOptionEnum_MaaGlobalOption_LogDir as i32,
        c_dir.as_ptr() as *mut _,
        c_dir.as_bytes().len() as u64,
    )
}

/// Enable or disable debug mode.
///
/// In debug mode:
/// - Recognition details include raw images and draws
/// - All tasks are treated as focus tasks and produce callbacks
///
/// # Arguments
/// * `enable` - `true` to enable debug mode
pub fn set_debug_mode(enable: bool) -> MaaResult<()> {
    let mut val_bool = if enable { 1u8 } else { 0u8 };
    set_global_option(
        sys::MaaGlobalOptionEnum_MaaGlobalOption_DebugMode as i32,
        &mut val_bool as *mut _ as *mut _,
        std::mem::size_of::<u8>() as u64,
    )
}

/// Set the log level for stdout output.
///
/// # Arguments
/// * `level` - Logging level (use `sys::MaaLoggingLevel*` constants)
pub fn set_stdout_level(level: sys::MaaLoggingLevel) -> MaaResult<()> {
    let mut val = level;
    set_global_option(
        sys::MaaGlobalOptionEnum_MaaGlobalOption_StdoutLevel as i32,
        &mut val as *mut _ as *mut _,
        std::mem::size_of::<sys::MaaLoggingLevel>() as u64,
    )
}

/// Enable/disable saving recognition visualizations to log directory.
pub fn set_save_draw(enable: bool) -> MaaResult<()> {
    let mut val: u8 = if enable { 1 } else { 0 };
    set_global_option(
        sys::MaaGlobalOptionEnum_MaaGlobalOption_SaveDraw as i32,
        &mut val as *mut _ as *mut _,
        std::mem::size_of::<u8>() as u64,
    )
}

/// Enable/disable saving screenshots on error.
pub fn set_save_on_error(enable: bool) -> MaaResult<()> {
    let mut val: u8 = if enable { 1 } else { 0 };
    set_global_option(
        sys::MaaGlobalOptionEnum_MaaGlobalOption_SaveOnError as i32,
        &mut val as *mut _ as *mut _,
        std::mem::size_of::<u8>() as u64,
    )
}

/// Set JPEG quality for saved draw images (0-100, default 85).
pub fn set_draw_quality(quality: i32) -> MaaResult<()> {
    let mut val = quality;
    set_global_option(
        sys::MaaGlobalOptionEnum_MaaGlobalOption_DrawQuality as i32,
        &mut val as *mut _ as *mut _,
        std::mem::size_of::<i32>() as u64,
    )
}

/// Set the recognition image cache limit (default 4096).
pub fn set_reco_image_cache_limit(limit: u64) -> MaaResult<()> {
    let mut val = limit;
    set_global_option(
        sys::MaaGlobalOptionEnum_MaaGlobalOption_RecoImageCacheLimit as i32,
        &mut val as *mut _ as *mut _,
        std::mem::size_of::<u64>() as u64,
    )
}

/// Load a plugin from the specified path.
pub fn load_plugin(path: &str) -> MaaResult<()> {
    let c_path = CString::new(path)?;
    let ret = unsafe { sys::MaaGlobalLoadPlugin(c_path.as_ptr()) };
    common::check_bool(ret)
}
