use crate::buffer::{ImageBuffer, StringBuffer, StringListBuffer};
use crate::custom::{CustomAction, CustomRecognition};
use crate::define::*;
use crate::error::{MaaError, Result};
use crate::ffi;
use crate::job::ResJob;
use serde_json::Value;
use std::collections::HashMap;
use std::ffi::CString;
use std::path::Path;
use std::sync::Arc;

pub struct Resource {
    handle: MaaResourceHandle,
    own: bool,
    custom_recognitions: HashMap<String, Arc<dyn CustomRecognition>>,
    custom_actions: HashMap<String, Arc<dyn CustomAction>>,
}

impl Resource {
    pub fn new() -> Result<Self> {
        let handle = ffi::maa_resource_create();
        if handle.is_null() {
            return Err(MaaError::CreateFailed("Resource"));
        }
        Ok(Self {
            handle,
            own: true,
            custom_recognitions: HashMap::new(),
            custom_actions: HashMap::new(),
        })
    }

    pub(crate) fn from_handle(handle: MaaResourceHandle) -> Self {
        Self {
            handle,
            own: false,
            custom_recognitions: HashMap::new(),
            custom_actions: HashMap::new(),
        }
    }

    pub fn handle(&self) -> MaaResourceHandle {
        self.handle
    }

    pub fn post_bundle(&self, path: impl AsRef<Path>) -> ResJob {
        let path_str = path.as_ref().to_string_lossy();
        let cstr = CString::new(path_str.as_ref()).unwrap();
        let id = ffi::maa_resource_post_bundle(self.handle, cstr.as_ptr());
        ResJob::new(id, self.handle)
    }

    pub fn override_pipeline(&self, pipeline: &Value) -> Result<bool> {
        let json_str = serde_json::to_string(pipeline)?;
        let cstr = CString::new(json_str).map_err(|e| MaaError::BufferError(e.to_string()))?;
        Ok(ffi::maa_resource_override_pipeline(self.handle, cstr.as_ptr()) != 0)
    }

    pub fn override_next(&self, name: &str, next_list: &[&str]) -> Result<bool> {
        let name_cstr = CString::new(name).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let list_buffer = StringListBuffer::new();
        list_buffer.set(next_list)?;
        Ok(ffi::maa_resource_override_next(self.handle, name_cstr.as_ptr(), list_buffer.handle())
            != 0)
    }

    pub fn override_image(&self, image_name: &str, image: &ImageBuffer) -> Result<bool> {
        let name_cstr =
            CString::new(image_name).map_err(|e| MaaError::BufferError(e.to_string()))?;
        Ok(
            ffi::maa_resource_override_image(self.handle, name_cstr.as_ptr(), image.handle())
                != 0,
        )
    }

    pub fn get_node_data(&self, name: &str) -> Result<Option<Value>> {
        let name_cstr = CString::new(name).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let buffer = StringBuffer::new();
        if ffi::maa_resource_get_node_data(self.handle, name_cstr.as_ptr(), buffer.handle()) == 0 {
            return Ok(None);
        }
        let data = buffer.get()?;
        if data.is_empty() {
            return Ok(None);
        }
        let value: Value = serde_json::from_str(&data)?;
        Ok(Some(value))
    }

    pub fn loaded(&self) -> bool {
        ffi::maa_resource_loaded(self.handle) != 0
    }

    pub fn clear(&self) -> bool {
        ffi::maa_resource_clear(self.handle) != 0
    }

    pub fn use_cpu(&self) -> bool {
        self.set_inference(InferenceExecutionProvider::Cpu, InferenceDevice::Cpu)
    }

    pub fn use_directml(&self, device_id: i32) -> bool {
        self.set_inference(InferenceExecutionProvider::DirectMl, unsafe {
            std::mem::transmute(device_id)
        })
    }

    pub fn use_coreml(&self, coreml_flag: i32) -> bool {
        self.set_inference(InferenceExecutionProvider::CoreMl, unsafe {
            std::mem::transmute(coreml_flag)
        })
    }

    pub fn use_auto_ep(&self) -> bool {
        self.set_inference(InferenceExecutionProvider::Auto, InferenceDevice::Auto)
    }

    fn set_inference(&self, ep: InferenceExecutionProvider, device: InferenceDevice) -> bool {
        let ep_val = ep as i32;
        let device_val = device as i32;
        let ep_ret = ffi::maa_resource_set_option(
            self.handle,
            ResOption::InferenceExecutionProvider as i32,
            &ep_val as *const i32 as *mut std::ffi::c_void,
            std::mem::size_of::<i32>() as u64,
        );
        let device_ret = ffi::maa_resource_set_option(
            self.handle,
            ResOption::InferenceDevice as i32,
            &device_val as *const i32 as *mut std::ffi::c_void,
            std::mem::size_of::<i32>() as u64,
        );
        ep_ret != 0 && device_ret != 0
    }

    pub fn hash(&self) -> Result<String> {
        let buffer = StringBuffer::new();
        if ffi::maa_resource_get_hash(self.handle, buffer.handle()) == 0 {
            return Err(MaaError::OperationFailed("get hash"));
        }
        buffer.get()
    }

    pub fn node_list(&self) -> Result<Vec<String>> {
        let buffer = StringListBuffer::new();
        if ffi::maa_resource_get_node_list(self.handle, buffer.handle()) == 0 {
            return Err(MaaError::OperationFailed("get node list"));
        }
        buffer.get()
    }

    pub fn custom_recognition_list(&self) -> Result<Vec<String>> {
        let buffer = StringListBuffer::new();
        if ffi::maa_resource_get_custom_recognition_list(self.handle, buffer.handle()) == 0 {
            return Err(MaaError::OperationFailed("get custom recognition list"));
        }
        buffer.get()
    }

    pub fn custom_action_list(&self) -> Result<Vec<String>> {
        let buffer = StringListBuffer::new();
        if ffi::maa_resource_get_custom_action_list(self.handle, buffer.handle()) == 0 {
            return Err(MaaError::OperationFailed("get custom action list"));
        }
        buffer.get()
    }
}

impl Drop for Resource {
    fn drop(&mut self) {
        if self.own && !self.handle.is_null() {
            ffi::maa_resource_destroy(self.handle);
        }
    }
}

unsafe impl Send for Resource {}
unsafe impl Sync for Resource {}

