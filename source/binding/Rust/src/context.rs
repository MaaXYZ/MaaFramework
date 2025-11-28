use crate::buffer::{ImageBuffer, RectBuffer, StringBuffer, StringListBuffer};
use crate::define::*;
use crate::error::{MaaError, Result};
use crate::ffi;
use crate::job::TaskJob;
use crate::tasker::{ActionDetail, RecognitionDetail, TaskDetail, Tasker};
use serde_json::Value;
use std::ffi::CString;

pub struct Context {
    handle: MaaContextHandle,
    tasker: Tasker,
}

impl Context {
    pub fn from_handle(handle: MaaContextHandle) -> Result<Self> {
        if handle.is_null() {
            return Err(MaaError::InvalidHandle);
        }
        let tasker_handle = ffi::maa_context_get_tasker(handle);
        if tasker_handle.is_null() {
            return Err(MaaError::InvalidHandle);
        }
        Ok(Self {
            handle,
            tasker: Tasker::from_handle(tasker_handle),
        })
    }

    pub fn handle(&self) -> MaaContextHandle {
        self.handle
    }

    pub fn run_task(&self, entry: &str, pipeline_override: &Value) -> Result<Option<TaskDetail>> {
        let entry_cstr = CString::new(entry).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let override_str = serde_json::to_string(pipeline_override)?;
        let override_cstr =
            CString::new(override_str).map_err(|e| MaaError::BufferError(e.to_string()))?;

        let task_id =
            ffi::maa_context_run_task(self.handle, entry_cstr.as_ptr(), override_cstr.as_ptr());

        if task_id == MAA_INVALID_ID {
            return Ok(None);
        }

        self.tasker.get_task_detail(task_id)
    }

    pub fn run_recognition(
        &self,
        entry: &str,
        image: &ImageBuffer,
        pipeline_override: &Value,
    ) -> Result<Option<RecognitionDetail>> {
        let entry_cstr = CString::new(entry).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let override_str = serde_json::to_string(pipeline_override)?;
        let override_cstr =
            CString::new(override_str).map_err(|e| MaaError::BufferError(e.to_string()))?;

        let reco_id = ffi::maa_context_run_recognition(
            self.handle,
            entry_cstr.as_ptr(),
            override_cstr.as_ptr(),
            image.handle(),
        );

        if reco_id == MAA_INVALID_ID {
            return Ok(None);
        }

        self.tasker.get_recognition_detail(reco_id)
    }

    pub fn run_action(
        &self,
        entry: &str,
        box_: &Rect,
        reco_detail: &str,
        pipeline_override: &Value,
    ) -> Result<Option<ActionDetail>> {
        let entry_cstr = CString::new(entry).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let override_str = serde_json::to_string(pipeline_override)?;
        let override_cstr =
            CString::new(override_str).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let reco_cstr =
            CString::new(reco_detail).map_err(|e| MaaError::BufferError(e.to_string()))?;

        let rect_buf = RectBuffer::new();
        rect_buf.set(box_);

        let action_id = ffi::maa_context_run_action(
            self.handle,
            entry_cstr.as_ptr(),
            override_cstr.as_ptr(),
            rect_buf.handle(),
            reco_cstr.as_ptr(),
        );

        if action_id == MAA_INVALID_ID {
            return Ok(None);
        }

        self.tasker.get_action_detail(action_id)
    }

    pub fn override_pipeline(&self, pipeline_override: &Value) -> Result<bool> {
        let json_str = serde_json::to_string(pipeline_override)?;
        let cstr = CString::new(json_str).map_err(|e| MaaError::BufferError(e.to_string()))?;
        Ok(ffi::maa_context_override_pipeline(self.handle, cstr.as_ptr()) != 0)
    }

    pub fn override_next(&self, name: &str, next_list: &[&str]) -> Result<bool> {
        let name_cstr = CString::new(name).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let list_buffer = StringListBuffer::new();
        list_buffer.set(next_list)?;
        Ok(
            ffi::maa_context_override_next(self.handle, name_cstr.as_ptr(), list_buffer.handle())
                != 0,
        )
    }

    pub fn override_image(&self, image_name: &str, image: &ImageBuffer) -> Result<bool> {
        let name_cstr =
            CString::new(image_name).map_err(|e| MaaError::BufferError(e.to_string()))?;
        Ok(
            ffi::maa_context_override_image(self.handle, name_cstr.as_ptr(), image.handle())
                != 0,
        )
    }

    pub fn get_node_data(&self, name: &str) -> Result<Option<Value>> {
        let name_cstr = CString::new(name).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let buffer = StringBuffer::new();
        if ffi::maa_context_get_node_data(self.handle, name_cstr.as_ptr(), buffer.handle()) == 0 {
            return Ok(None);
        }
        let data = buffer.get()?;
        if data.is_empty() {
            return Ok(None);
        }
        let value: Value = serde_json::from_str(&data)?;
        Ok(Some(value))
    }

    pub fn task_id(&self) -> MaaTaskId {
        ffi::maa_context_get_task_id(self.handle)
    }

    pub fn tasker(&self) -> &Tasker {
        &self.tasker
    }

    pub fn get_task_job(&self) -> TaskJob {
        let task_id = self.task_id();
        TaskJob::new(task_id, self.tasker.handle())
    }

    pub fn clone_context(&self) -> Result<Self> {
        let handle = ffi::maa_context_clone(self.handle);
        if handle.is_null() {
            return Err(MaaError::OperationFailed("clone context"));
        }
        Context::from_handle(handle)
    }

    pub fn set_anchor(&self, anchor_name: &str, node_name: &str) -> Result<bool> {
        let anchor_cstr =
            CString::new(anchor_name).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let node_cstr =
            CString::new(node_name).map_err(|e| MaaError::BufferError(e.to_string()))?;
        Ok(
            ffi::maa_context_set_anchor(self.handle, anchor_cstr.as_ptr(), node_cstr.as_ptr())
                != 0,
        )
    }

    pub fn get_anchor(&self, anchor_name: &str) -> Result<Option<String>> {
        let anchor_cstr =
            CString::new(anchor_name).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let buffer = StringBuffer::new();
        if ffi::maa_context_get_anchor(self.handle, anchor_cstr.as_ptr(), buffer.handle()) == 0 {
            return Ok(None);
        }
        let name = buffer.get()?;
        Ok(Some(name))
    }

    pub fn get_hit_count(&self, node_name: &str) -> Result<u64> {
        let node_cstr =
            CString::new(node_name).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let mut count: u64 = 0;
        if ffi::maa_context_get_hit_count(self.handle, node_cstr.as_ptr(), &mut count) == 0 {
            return Ok(0);
        }
        Ok(count)
    }

    pub fn clear_hit_count(&self, node_name: &str) -> Result<bool> {
        let node_cstr =
            CString::new(node_name).map_err(|e| MaaError::BufferError(e.to_string()))?;
        Ok(ffi::maa_context_clear_hit_count(self.handle, node_cstr.as_ptr()) != 0)
    }
}

