//! Custom recognizer and action components.
//!
//! This module provides traits for implementing custom recognition algorithms
//! and actions that integrate with the MaaFramework pipeline system.
//!
//! # Usage
//!
//! 1. Implement [`CustomRecognition`] or [`CustomAction`] trait
//! 2. Register with [`Resource::register_custom_recognition`] or [`Resource::register_custom_action`]
//! 3. Reference in pipeline JSON via `"recognition": "Custom"` or `"action": "Custom"`
//!
//! # Example
//!
//! ```ignore
//! struct MyRecognizer;
//!
//! impl CustomRecognition for MyRecognizer {
//!     fn analyze(&self, context: &Context, ...) -> Option<(MaaRect, String)> {
//!         // Your recognition logic
//!         Some((rect, "{}" .to_string()))
//!     }
//! }
//!
//! resource.register_custom_recognition("MyReco", Box::new(MyRecognizer))?;
//! ```

use crate::context::Context;
use crate::resource::Resource;
use crate::{common, sys, MaaError, MaaResult};
use std::ffi::{CStr, CString};
use std::os::raw::c_void;

// === Structured Arguments ===

/// Arguments passed to custom action's run method.
#[derive(Debug, Clone)]
pub struct RunArg {
    /// The current task ID.
    pub task_id: common::MaaId,
    /// Name of the current node in the pipeline.
    pub node_name: String,
    /// Name of the custom action.
    pub action_name: String,
    /// JSON parameters for the action.
    pub action_param: String,
    /// Recognition ID from the preceding recognition.
    pub reco_id: common::MaaId,
    /// Bounding box from the recognition result.
    pub box_rect: common::Rect,
}

/// Arguments passed to custom recognition's analyze method.
#[derive(Debug, Clone)]
pub struct AnalyzeArg {
    /// The current task ID.
    pub task_id: common::MaaId,
    /// Name of the current node in the pipeline.
    pub node_name: String,
    /// Name of the custom recognition.
    pub reco_name: String,
    /// JSON parameters for the recognition.
    pub reco_param: String,
    /// Region of interest to analyze.
    pub roi: common::Rect,
}

/// Result returned from custom action's run method.
#[derive(Debug, Clone, Default)]
pub struct RunResult {
    /// Whether the action succeeded.
    pub success: bool,
}

impl RunResult {
    pub fn success() -> Self {
        Self { success: true }
    }

    pub fn failure() -> Self {
        Self { success: false }
    }
}

/// Result returned from custom recognition's analyze method.
#[derive(Debug, Clone)]
pub struct AnalyzeResult {
    /// The bounding box of the detected target.
    pub box_rect: common::Rect,
    /// JSON detail string to be passed to subsequent actions.
    pub detail: String,
}

impl AnalyzeResult {
    pub fn new(box_rect: common::Rect, detail: impl Into<String>) -> Self {
        Self {
            box_rect,
            detail: detail.into(),
        }
    }
}

/// Trait for implementing custom actions.
///
/// Actions are executed after a successful recognition and can perform
/// clicks, swipes, or custom logic.
pub trait CustomAction: Send + Sync {
    /// Execute the custom action.
    ///
    /// # Returns
    /// `true` if the action succeeded, `false` otherwise
    fn run(
        &self,
        context: &Context,
        task_id: sys::MaaTaskId,
        node_name: &str,
        custom_action_name: &str,
        custom_action_param: &str,
        reco_id: sys::MaaRecoId,
        box_rect: &sys::MaaRect,
    ) -> bool;
}

/// Trait for implementing custom recognizers.
///
/// Recognizers analyze screenshots to find targets in the UI.
pub trait CustomRecognition: Send + Sync {
    /// Analyze an image to find the target.
    ///
    /// # Returns
    /// `Some((rect, detail))` if target found, `None` otherwise
    fn analyze(
        &self,
        context: &Context,
        task_id: sys::MaaTaskId,
        node_name: &str,
        custom_recognition_name: &str,
        custom_recognition_param: &str,
        image: *const sys::MaaImageBuffer,
        roi: &sys::MaaRect,
    ) -> Option<(sys::MaaRect, String)>;
}

// === Enhanced Traits (Alternative API) ===

/// Enhanced custom action trait with structured arguments.
pub trait CustomActionV2: Send + Sync {
    /// Run the custom action with structured arguments.
    fn run(&self, context: &Context, arg: &RunArg) -> RunResult;
}

/// Enhanced custom recognition trait with structured arguments.
pub trait CustomRecognitionV2: Send + Sync {
    /// Analyze the image with structured arguments.
    /// Returns None if recognition failed, Some(result) if target found.
    fn analyze(
        &self,
        context: &Context,
        arg: &AnalyzeArg,
        image: &crate::buffer::MaaImageBuffer,
    ) -> Option<AnalyzeResult>;
}

// === Function Wrappers ===

/// Wrapper to use a closure as a custom recognition.
///
/// This allows using a simple closure instead of implementing a full trait.
///
/// # Example
/// ```ignore
/// use maa_framework::custom::{FnRecognition, AnalyzeArg, AnalyzeResult};
///
/// let my_reco = FnRecognition::new(|ctx, arg, img| {
///     // Custom recognition logic
///     Some(AnalyzeResult::new(
///         crate::common::Rect { x: 100, y: 100, width: 50, height: 50 },
///         "{}",
///     ))
/// });
/// ```
pub struct FnRecognition<F>
where
    F: Fn(&Context, &AnalyzeArg, &crate::buffer::MaaImageBuffer) -> Option<AnalyzeResult>
        + Send
        + Sync,
{
    func: F,
}

impl<F> FnRecognition<F>
where
    F: Fn(&Context, &AnalyzeArg, &crate::buffer::MaaImageBuffer) -> Option<AnalyzeResult>
        + Send
        + Sync,
{
    /// Create a new FnRecognition from a closure.
    pub fn new(func: F) -> Self {
        Self { func }
    }
}

impl<F> CustomRecognitionV2 for FnRecognition<F>
where
    F: Fn(&Context, &AnalyzeArg, &crate::buffer::MaaImageBuffer) -> Option<AnalyzeResult>
        + Send
        + Sync,
{
    fn analyze(
        &self,
        context: &Context,
        arg: &AnalyzeArg,
        image: &crate::buffer::MaaImageBuffer,
    ) -> Option<AnalyzeResult> {
        (self.func)(context, arg, image)
    }
}

/// Wrapper to use a closure as a custom action.
///
/// This allows using a simple closure instead of implementing a full trait.
///
/// # Example
/// ```ignore
/// use maa_framework::custom::{FnAction, RunArg, RunResult};
///
/// let my_action = FnAction::new(|ctx, arg| {
///     println!("Running action on node: {}", arg.node_name);
///     RunResult::success()
/// });
/// ```
pub struct FnAction<F>
where
    F: Fn(&Context, &RunArg) -> RunResult + Send + Sync,
{
    func: F,
}

impl<F> FnAction<F>
where
    F: Fn(&Context, &RunArg) -> RunResult + Send + Sync,
{
    /// Create a new FnAction from a closure.
    pub fn new(func: F) -> Self {
        Self { func }
    }
}

impl<F> CustomActionV2 for FnAction<F>
where
    F: Fn(&Context, &RunArg) -> RunResult + Send + Sync,
{
    fn run(&self, context: &Context, arg: &RunArg) -> RunResult {
        (self.func)(context, arg)
    }
}

pub(crate) unsafe extern "C" fn custom_action_trampoline(
    context: *mut sys::MaaContext,
    task_id: sys::MaaTaskId,
    node_name: *const std::os::raw::c_char,
    custom_action_name: *const std::os::raw::c_char,
    custom_action_param: *const std::os::raw::c_char,
    reco_id: sys::MaaRecoId,
    box_: *const sys::MaaRect,
    trans_arg: *mut std::os::raw::c_void,
) -> sys::MaaBool {
    if trans_arg.is_null() {
        return sys::MaaStatusEnum_MaaStatus_Failed as sys::MaaBool;
    }

    let action_box = trans_arg as *mut Box<dyn CustomAction>;
    let action = &**action_box;

    let ctx = match Context::from_raw(context) {
        Some(c) => c,
        None => return 0,
    };

    let c_node = CStr::from_ptr(node_name).to_string_lossy();
    let c_name = CStr::from_ptr(custom_action_name).to_string_lossy();
    let c_param = CStr::from_ptr(custom_action_param).to_string_lossy();

    let rect = if !box_.is_null() {
        *box_
    } else {
        sys::MaaRect {
            x: 0,
            y: 0,
            width: 0,
            height: 0,
        }
    };

    let result = action.run(&ctx, task_id, &c_node, &c_name, &c_param, reco_id, &rect);

    if result {
        1
    } else {
        0
    }
}

pub(crate) unsafe extern "C" fn custom_recognition_trampoline(
    context: *mut sys::MaaContext,
    task_id: sys::MaaTaskId,
    node_name: *const std::os::raw::c_char,
    custom_recognition_name: *const std::os::raw::c_char,
    custom_recognition_param: *const std::os::raw::c_char,
    image: *const sys::MaaImageBuffer,
    roi: *const sys::MaaRect,
    trans_arg: *mut std::os::raw::c_void,
    out_box: *mut sys::MaaRect,
    out_detail: *mut sys::MaaStringBuffer,
) -> sys::MaaBool {
    if trans_arg.is_null() {
        return 0;
    }
    let reco_box = trans_arg as *mut Box<dyn CustomRecognition>;
    let reco = &**reco_box;

    let ctx = match Context::from_raw(context) {
        Some(c) => c,
        None => return 0,
    };

    let c_node = CStr::from_ptr(node_name).to_string_lossy();
    let c_name = CStr::from_ptr(custom_recognition_name).to_string_lossy();
    let c_param = CStr::from_ptr(custom_recognition_param).to_string_lossy();

    let rect = if !roi.is_null() {
        *roi
    } else {
        sys::MaaRect {
            x: 0,
            y: 0,
            width: 0,
            height: 0,
        }
    };

    let result = reco.analyze(&ctx, task_id, &c_node, &c_name, &c_param, image, &rect);

    if let Some((res_rect, res_detail)) = result {
        if !out_box.is_null() {
            *out_box = res_rect;
        }
        if !out_detail.is_null() {
            let c_detail = CString::new(res_detail).unwrap_or_default();
            sys::MaaStringBufferSet(out_detail, c_detail.as_ptr());
        }
        1
    } else {
        0
    }
}

impl Resource {
    /// Register a custom action with the resource.
    ///
    /// After registration, the action can be referenced in pipeline JSON:
    /// ```json
    /// {
    ///     "action": "Custom",
    ///     "custom_action": "MyAction"
    /// }
    /// ```
    pub fn register_custom_action(
        &self,
        name: &str,
        action: Box<dyn CustomAction>,
    ) -> MaaResult<()> {
        let c_name = CString::new(name)?;
        let action_ptr = Box::into_raw(Box::new(action));
        let action_ptr_void = action_ptr as *mut c_void;

        unsafe {
            let ret = sys::MaaResourceRegisterCustomAction(
                self.handle.as_ptr(),
                c_name.as_ptr(),
                Some(custom_action_trampoline),
                action_ptr_void,
            );
            if ret == 0 {
                let _ = Box::from_raw(action_ptr);
                return Err(MaaError::FrameworkError(0));
            }
        }

        let mut map = self.custom_actions.lock().unwrap();
        if let Some(old_ptr) = map.insert(name.to_string(), action_ptr as usize) {
            unsafe {
                let _ = Box::from_raw(old_ptr as *mut Box<dyn CustomAction>);
            }
        }

        Ok(())
    }

    /// Register a custom recognition with the resource.
    ///
    /// After registration, the recognizer can be referenced in pipeline JSON:
    /// ```json
    /// {
    ///     "recognition": "Custom",
    ///     "custom_recognition": "MyReco"
    /// }
    /// ```
    pub fn register_custom_recognition(
        &self,
        name: &str,
        reco: Box<dyn CustomRecognition>,
    ) -> MaaResult<()> {
        let c_name = CString::new(name)?;
        let reco_ptr = Box::into_raw(Box::new(reco));
        let reco_ptr_void = reco_ptr as *mut c_void;

        unsafe {
            let ret = sys::MaaResourceRegisterCustomRecognition(
                self.handle.as_ptr(),
                c_name.as_ptr(),
                Some(custom_recognition_trampoline),
                reco_ptr_void,
            );
            if ret == 0 {
                let _ = Box::from_raw(reco_ptr);
                return Err(MaaError::FrameworkError(0));
            }
        }

        let mut map = self.custom_recognitions.lock().unwrap();
        if let Some(old_ptr) = map.insert(name.to_string(), reco_ptr as usize) {
            unsafe {
                let _ = Box::from_raw(old_ptr as *mut Box<dyn CustomRecognition>);
            }
        }

        Ok(())
    }
}
