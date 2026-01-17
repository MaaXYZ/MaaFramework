use std::ffi::CString;
use std::ptr::NonNull;

use crate::{common, sys, MaaError, MaaResult};

/// Resource manager.
///
/// Handles loading and management of:
/// - Image resources
/// - OCR models
/// - Task pipelines
/// - Custom recognizers and actions
pub struct Resource {
    pub(crate) handle: NonNull<sys::MaaResource>,
    pub(crate) custom_actions: std::sync::Mutex<std::collections::HashMap<String, usize>>, // Store pointer address
    pub(crate) custom_recognitions: std::sync::Mutex<std::collections::HashMap<String, usize>>,
}

unsafe impl Send for Resource {}
unsafe impl Sync for Resource {}

impl Resource {
    /// Create a new resource manager.
    pub fn new() -> MaaResult<Self> {
        let handle = unsafe { sys::MaaResourceCreate() };
        if let Some(ptr) = NonNull::new(handle) {
            Ok(Self {
                handle: ptr,
                custom_actions: std::sync::Mutex::new(std::collections::HashMap::new()),
                custom_recognitions: std::sync::Mutex::new(std::collections::HashMap::new()),
            })
        } else {
            Err(MaaError::NullPointer)
        }
    }

    /// Load a resource bundle from the specified directory.
    ///
    /// The bundle should contain pipeline definitions, images, and models.
    pub fn post_bundle(&self, path: &str) -> MaaResult<i64> {
        let c_path = CString::new(path)?;
        let id = unsafe { sys::MaaResourcePostBundle(self.handle.as_ptr(), c_path.as_ptr()) };
        Ok(id)
    }

    /// Check if resources have been loaded.
    pub fn loaded(&self) -> bool {
        unsafe { sys::MaaResourceLoaded(self.handle.as_ptr()) != 0 }
    }

    /// Clear all loaded resources.
    pub fn clear(&self) -> MaaResult<()> {
        let ret = unsafe { sys::MaaResourceClear(self.handle.as_ptr()) };
        common::check_bool(ret)
    }

    /// Get the status of a loading operation.
    pub fn status(&self, id: common::MaaId) -> common::MaaStatus {
        let status = unsafe { sys::MaaResourceStatus(self.handle.as_ptr(), id) };
        common::MaaStatus(status)
    }

    /// Wait for a loading operation to complete.
    pub fn wait(&self, id: common::MaaId) -> common::MaaStatus {
        let status = unsafe { sys::MaaResourceWait(self.handle.as_ptr(), id) };
        common::MaaStatus(status)
    }

    /// Get the raw resource handle.
    pub fn raw(&self) -> *mut sys::MaaResource {
        self.handle.as_ptr()
    }

    // === Additional resource loading ===

    pub fn post_ocr_model(&self, path: &str) -> MaaResult<i64> {
        let c_path = CString::new(path)?;
        let id = unsafe { sys::MaaResourcePostOcrModel(self.handle.as_ptr(), c_path.as_ptr()) };
        Ok(id)
    }

    pub fn post_pipeline(&self, path: &str) -> MaaResult<i64> {
        let c_path = CString::new(path)?;
        let id = unsafe { sys::MaaResourcePostPipeline(self.handle.as_ptr(), c_path.as_ptr()) };
        Ok(id)
    }

    pub fn post_image(&self, path: &str) -> MaaResult<i64> {
        let c_path = CString::new(path)?;
        let id = unsafe { sys::MaaResourcePostImage(self.handle.as_ptr(), c_path.as_ptr()) };
        Ok(id)
    }

    // === Pipeline operations ===

    pub fn override_pipeline(&self, pipeline_override: &str) -> MaaResult<()> {
        let c_json = CString::new(pipeline_override)?;
        let ret =
            unsafe { sys::MaaResourceOverridePipeline(self.handle.as_ptr(), c_json.as_ptr()) };
        common::check_bool(ret)
    }

    /// Override pipeline with JSON value.
    ///
    /// Convenience method that accepts `serde_json::Value` for pipeline overrides.
    ///
    /// # Example
    /// ```ignore
    /// use serde_json::json;
    /// resource.override_pipeline_json(&json!({
    ///     "MyNode": { "enabled": false }
    /// }))?;
    /// ```
    pub fn override_pipeline_json(&self, pipeline_override: &serde_json::Value) -> MaaResult<()> {
        self.override_pipeline(&pipeline_override.to_string())
    }

    pub fn override_next(&self, node_name: &str, next_list: &[&str]) -> MaaResult<()> {
        let c_name = CString::new(node_name)?;
        let list_buf = crate::buffer::MaaStringListBuffer::new()?;
        for item in next_list {
            list_buf.append(item)?;
        }
        let ret = unsafe {
            sys::MaaResourceOverrideNext(self.handle.as_ptr(), c_name.as_ptr(), list_buf.raw())
        };
        common::check_bool(ret)
    }

    pub fn get_node_data(&self, node_name: &str) -> MaaResult<Option<String>> {
        let c_name = CString::new(node_name)?;
        let buffer = crate::buffer::MaaStringBuffer::new()?;
        let ret = unsafe {
            sys::MaaResourceGetNodeData(self.handle.as_ptr(), c_name.as_ptr(), buffer.raw())
        };
        if ret != 0 {
            Ok(Some(buffer.to_string()))
        } else {
            Ok(None)
        }
    }

    pub fn get_node_object(
        &self,
        node_name: &str,
    ) -> MaaResult<Option<crate::pipeline::PipelineData>> {
        if let Some(json_str) = self.get_node_data(node_name)? {
            let data: crate::pipeline::PipelineData =
                serde_json::from_str(&json_str).map_err(|e| {
                    MaaError::InvalidConfig(format!("Failed to parse pipeline data: {}", e))
                })?;
            Ok(Some(data))
        } else {
            Ok(None)
        }
    }

    pub fn node_list(&self) -> MaaResult<Vec<String>> {
        let buffer = crate::buffer::MaaStringListBuffer::new()?;
        let ret = unsafe { sys::MaaResourceGetNodeList(self.handle.as_ptr(), buffer.raw()) };
        if ret != 0 {
            Ok(buffer.to_vec())
        } else {
            Err(MaaError::FrameworkError(0))
        }
    }

    pub fn hash(&self) -> MaaResult<String> {
        let buffer = crate::buffer::MaaStringBuffer::new()?;
        let ret = unsafe { sys::MaaResourceGetHash(self.handle.as_ptr(), buffer.raw()) };
        if ret != 0 {
            Ok(buffer.to_string())
        } else {
            Err(MaaError::FrameworkError(0))
        }
    }

    // === Inference device ===

    pub fn use_cpu(&self) -> MaaResult<()> {
        let mut device: i32 = sys::MaaInferenceDeviceEnum_MaaInferenceDevice_CPU as i32;
        let ret = unsafe {
            sys::MaaResourceSetOption(
                self.handle.as_ptr(),
                sys::MaaResOptionEnum_MaaResOption_InferenceDevice as i32,
                &mut device as *mut _ as *mut std::ffi::c_void,
                std::mem::size_of::<i32>() as u64,
            )
        };
        common::check_bool(ret)
    }

    pub fn use_directml(&self, device_id: i32) -> MaaResult<()> {
        let mut ep: i32 =
            sys::MaaInferenceExecutionProviderEnum_MaaInferenceExecutionProvider_DirectML as i32;
        let ret1 = unsafe {
            sys::MaaResourceSetOption(
                self.handle.as_ptr(),
                sys::MaaResOptionEnum_MaaResOption_InferenceExecutionProvider as i32,
                &mut ep as *mut _ as *mut std::ffi::c_void,
                std::mem::size_of::<i32>() as u64,
            )
        };
        common::check_bool(ret1)?;

        let mut device = device_id;
        let ret2 = unsafe {
            sys::MaaResourceSetOption(
                self.handle.as_ptr(),
                sys::MaaResOptionEnum_MaaResOption_InferenceDevice as i32,
                &mut device as *mut _ as *mut std::ffi::c_void,
                std::mem::size_of::<i32>() as u64,
            )
        };
        common::check_bool(ret2)
    }

    // === EventSink ===

    pub fn add_sink<F>(&self, callback: F) -> MaaResult<sys::MaaSinkId>
    where
        F: Fn(&str, &str) + Send + Sync + 'static,
    {
        let (cb_fn, cb_arg) = crate::callback::EventCallback::new(callback);
        let sink_id = unsafe { sys::MaaResourceAddSink(self.handle.as_ptr(), cb_fn, cb_arg) };
        if sink_id != 0 {
            Ok(sink_id)
        } else {
            unsafe { crate::callback::EventCallback::drop_callback(cb_arg) };
            Err(MaaError::FrameworkError(0))
        }
    }

    pub fn remove_sink(&self, sink_id: sys::MaaSinkId) {
        unsafe { sys::MaaResourceRemoveSink(self.handle.as_ptr(), sink_id) }
    }

    pub fn clear_sinks(&self) {
        unsafe { sys::MaaResourceClearSinks(self.handle.as_ptr()) }
    }

    // === Image override ===

    /// Override an image resource at runtime.
    ///
    /// # Arguments
    /// * `image_name` - The name of the image to override
    /// * `image` - The new image buffer to use
    pub fn override_image(
        &self,
        image_name: &str,
        image: &crate::buffer::MaaImageBuffer,
    ) -> MaaResult<()> {
        let c_name = CString::new(image_name)?;
        let ret = unsafe {
            sys::MaaResourceOverrideImage(self.handle.as_ptr(), c_name.as_ptr(), image.raw())
        };
        common::check_bool(ret)
    }

    // === Inference device (extended) ===

    /// Auto-select the best inference execution provider.
    pub fn use_auto_ep(&self) -> MaaResult<()> {
        let mut ep: i32 =
            sys::MaaInferenceExecutionProviderEnum_MaaInferenceExecutionProvider_Auto as i32;
        let ret1 = unsafe {
            sys::MaaResourceSetOption(
                self.handle.as_ptr(),
                sys::MaaResOptionEnum_MaaResOption_InferenceExecutionProvider as i32,
                &mut ep as *mut _ as *mut std::ffi::c_void,
                std::mem::size_of::<i32>() as u64,
            )
        };
        common::check_bool(ret1)?;

        let mut device: i32 = sys::MaaInferenceDeviceEnum_MaaInferenceDevice_Auto as i32;
        let ret2 = unsafe {
            sys::MaaResourceSetOption(
                self.handle.as_ptr(),
                sys::MaaResOptionEnum_MaaResOption_InferenceDevice as i32,
                &mut device as *mut _ as *mut std::ffi::c_void,
                std::mem::size_of::<i32>() as u64,
            )
        };
        common::check_bool(ret2)
    }

    /// Use CoreML for inference (macOS only).
    ///
    /// # Arguments
    /// * `coreml_flag` - CoreML configuration flag
    pub fn use_coreml(&self, coreml_flag: i32) -> MaaResult<()> {
        let mut ep: i32 =
            sys::MaaInferenceExecutionProviderEnum_MaaInferenceExecutionProvider_CoreML as i32;
        let ret1 = unsafe {
            sys::MaaResourceSetOption(
                self.handle.as_ptr(),
                sys::MaaResOptionEnum_MaaResOption_InferenceExecutionProvider as i32,
                &mut ep as *mut _ as *mut std::ffi::c_void,
                std::mem::size_of::<i32>() as u64,
            )
        };
        common::check_bool(ret1)?;

        let mut device = coreml_flag;
        let ret2 = unsafe {
            sys::MaaResourceSetOption(
                self.handle.as_ptr(),
                sys::MaaResOptionEnum_MaaResOption_InferenceDevice as i32,
                &mut device as *mut _ as *mut std::ffi::c_void,
                std::mem::size_of::<i32>() as u64,
            )
        };
        common::check_bool(ret2)
    }

    /// Use CUDA for inference (NVIDIA GPU only).
    ///
    /// # Arguments
    /// * `nvidia_gpu_id` - NVIDIA GPU device ID (typically 0 for first GPU)
    pub fn use_cuda(&self, nvidia_gpu_id: i32) -> MaaResult<()> {
        let mut ep: i32 =
            sys::MaaInferenceExecutionProviderEnum_MaaInferenceExecutionProvider_CUDA as i32;
        let ret1 = unsafe {
            sys::MaaResourceSetOption(
                self.handle.as_ptr(),
                sys::MaaResOptionEnum_MaaResOption_InferenceExecutionProvider as i32,
                &mut ep as *mut _ as *mut std::ffi::c_void,
                std::mem::size_of::<i32>() as u64,
            )
        };
        common::check_bool(ret1)?;

        let mut device = nvidia_gpu_id;
        let ret2 = unsafe {
            sys::MaaResourceSetOption(
                self.handle.as_ptr(),
                sys::MaaResOptionEnum_MaaResOption_InferenceDevice as i32,
                &mut device as *mut _ as *mut std::ffi::c_void,
                std::mem::size_of::<i32>() as u64,
            )
        };
        common::check_bool(ret2)
    }

    // === Custom component management ===

    /// Unregister a custom recognition by name.
    pub fn unregister_custom_recognition(&self, name: &str) -> MaaResult<()> {
        let c_name = CString::new(name)?;
        let ret = unsafe {
            sys::MaaResourceUnregisterCustomRecognition(self.handle.as_ptr(), c_name.as_ptr())
        };
        if ret != 0 {
            self.custom_recognitions.lock().unwrap().remove(name);
        }
        common::check_bool(ret)
    }

    /// Unregister a custom action by name.
    pub fn unregister_custom_action(&self, name: &str) -> MaaResult<()> {
        let c_name = CString::new(name)?;
        let ret = unsafe {
            sys::MaaResourceUnregisterCustomAction(self.handle.as_ptr(), c_name.as_ptr())
        };
        if ret != 0 {
            self.custom_actions.lock().unwrap().remove(name);
        }
        common::check_bool(ret)
    }

    /// Get the list of registered custom recognitions.
    pub fn custom_recognition_list(&self) -> MaaResult<Vec<String>> {
        let buffer = crate::buffer::MaaStringListBuffer::new()?;
        let ret =
            unsafe { sys::MaaResourceGetCustomRecognitionList(self.handle.as_ptr(), buffer.raw()) };
        if ret != 0 {
            Ok(buffer.to_vec())
        } else {
            Err(MaaError::FrameworkError(0))
        }
    }

    /// Get the list of registered custom actions.
    pub fn custom_action_list(&self) -> MaaResult<Vec<String>> {
        let buffer = crate::buffer::MaaStringListBuffer::new()?;
        let ret =
            unsafe { sys::MaaResourceGetCustomActionList(self.handle.as_ptr(), buffer.raw()) };
        if ret != 0 {
            Ok(buffer.to_vec())
        } else {
            Err(MaaError::FrameworkError(0))
        }
    }

    /// Clear all registered custom recognitions.
    pub fn clear_custom_recognition(&self) -> MaaResult<()> {
        let ret = unsafe { sys::MaaResourceClearCustomRecognition(self.handle.as_ptr()) };
        if ret != 0 {
            let mut recos = self.custom_recognitions.lock().unwrap();
            for (_, ptr) in recos.drain() {
                unsafe {
                    let _ = Box::from_raw(ptr as *mut Box<dyn crate::custom::CustomRecognition>);
                }
            }
        }
        common::check_bool(ret)
    }

    /// Clear all registered custom actions.
    pub fn clear_custom_action(&self) -> MaaResult<()> {
        let ret = unsafe { sys::MaaResourceClearCustomAction(self.handle.as_ptr()) };
        if ret != 0 {
            let mut actions = self.custom_actions.lock().unwrap();
            for (_, ptr) in actions.drain() {
                unsafe {
                    let _ = Box::from_raw(ptr as *mut Box<dyn crate::custom::CustomAction>);
                }
            }
        }
        common::check_bool(ret)
    }
}

impl Drop for Resource {
    fn drop(&mut self) {
        unsafe {
            sys::MaaResourceClearSinks(self.handle.as_ptr());
            sys::MaaResourceClearCustomAction(self.handle.as_ptr());
            sys::MaaResourceClearCustomRecognition(self.handle.as_ptr());

            {
                let mut actions = self.custom_actions.lock().unwrap();
                for (_, ptr) in actions.drain() {
                    let _ = Box::from_raw(ptr as *mut Box<dyn crate::custom::CustomAction>);
                }
            }
            {
                let mut recos = self.custom_recognitions.lock().unwrap();
                for (_, ptr) in recos.drain() {
                    let _ = Box::from_raw(ptr as *mut Box<dyn crate::custom::CustomRecognition>);
                }
            }

            sys::MaaResourceDestroy(self.handle.as_ptr())
        }
    }
}

/// A borrowed reference to a Resource.
///
/// This is a non-owning view that can be used for read-only operations.
/// It does NOT call destroy when dropped and should only be used while
/// the underlying Resource is still alive.
///
/// Obtained from `Tasker::resource()`.
pub struct ResourceRef<'a> {
    handle: *mut sys::MaaResource,
    _marker: std::marker::PhantomData<&'a ()>,
}

impl<'a> ResourceRef<'a> {
    pub(crate) fn from_ptr(handle: *mut sys::MaaResource) -> Option<Self> {
        if handle.is_null() {
            None
        } else {
            Some(Self {
                handle,
                _marker: std::marker::PhantomData,
            })
        }
    }

    /// Check if resources have been loaded.
    pub fn loaded(&self) -> bool {
        unsafe { sys::MaaResourceLoaded(self.handle) != 0 }
    }

    /// Get the status of a loading operation.
    pub fn status(&self, id: common::MaaId) -> common::MaaStatus {
        let status = unsafe { sys::MaaResourceStatus(self.handle, id) };
        common::MaaStatus(status)
    }

    /// Wait for a loading operation to complete.
    pub fn wait(&self, id: common::MaaId) -> common::MaaStatus {
        let status = unsafe { sys::MaaResourceWait(self.handle, id) };
        common::MaaStatus(status)
    }

    /// Get resource hash.
    pub fn hash(&self) -> MaaResult<String> {
        let buffer = crate::buffer::MaaStringBuffer::new()?;
        let ret = unsafe { sys::MaaResourceGetHash(self.handle, buffer.raw()) };
        if ret != 0 {
            Ok(buffer.to_string())
        } else {
            Err(MaaError::FrameworkError(0))
        }
    }

    /// Get node list.
    pub fn node_list(&self) -> MaaResult<Vec<String>> {
        let buffer = crate::buffer::MaaStringListBuffer::new()?;
        let ret = unsafe { sys::MaaResourceGetNodeList(self.handle, buffer.raw()) };
        if ret != 0 {
            Ok(buffer.to_vec())
        } else {
            Err(MaaError::FrameworkError(0))
        }
    }

    /// Get node data as JSON string.
    pub fn get_node_data(&self, node_name: &str) -> MaaResult<Option<String>> {
        let c_name = std::ffi::CString::new(node_name)?;
        let buffer = crate::buffer::MaaStringBuffer::new()?;
        let ret =
            unsafe { sys::MaaResourceGetNodeData(self.handle, c_name.as_ptr(), buffer.raw()) };
        if ret != 0 {
            Ok(Some(buffer.to_string()))
        } else {
            Ok(None)
        }
    }

    /// Get the raw handle.
    pub fn raw(&self) -> *mut sys::MaaResource {
        self.handle
    }
}
