use std::ffi::CString;
use std::ptr::NonNull;

use crate::{common, sys, MaaError, MaaResult};

/// Task execution context, internally held by Tasker.
///
/// Borrowed reference - no `MaaContextDestroy` in C API. Do not hold long-term.
pub struct Context {
    handle: NonNull<sys::MaaContext>,
    callbacks: std::sync::Mutex<std::collections::HashMap<sys::MaaSinkId, *mut std::ffi::c_void>>,
}

unsafe impl Send for Context {}
unsafe impl Sync for Context {}

impl Context {
    pub(crate) unsafe fn from_raw(ptr: *mut sys::MaaContext) -> Option<Self> {
        NonNull::new(ptr).map(|handle| Self {
            handle,
            callbacks: std::sync::Mutex::new(std::collections::HashMap::new()),
        })
    }

    pub fn run_task(&self, entry: &str, pipeline_override: &str) -> MaaResult<i64> {
        let c_entry = CString::new(entry)?;
        let c_pipeline = CString::new(pipeline_override)?;
        let id = unsafe {
            sys::MaaContextRunTask(self.handle.as_ptr(), c_entry.as_ptr(), c_pipeline.as_ptr())
        };
        Ok(id)
    }

    pub fn override_pipeline(&self, override_json: &str) -> MaaResult<()> {
        let c_json = CString::new(override_json)?;
        let ret = unsafe { sys::MaaContextOverridePipeline(self.handle.as_ptr(), c_json.as_ptr()) };
        common::check_bool(ret)
    }

    pub fn raw(&self) -> *mut sys::MaaContext {
        self.handle.as_ptr()
    }

    pub fn run_recognition(
        &self,
        entry: &str,
        pipeline_override: &str,
        image: &crate::buffer::MaaImageBuffer,
    ) -> MaaResult<i64> {
        let c_entry = CString::new(entry)?;
        let c_pipeline = CString::new(pipeline_override)?;
        let id = unsafe {
            sys::MaaContextRunRecognition(
                self.handle.as_ptr(),
                c_entry.as_ptr(),
                c_pipeline.as_ptr(),
                image.raw(),
            )
        };
        Ok(id)
    }

    pub fn run_action(
        &self,
        entry: &str,
        pipeline_override: &str,
        box_rect: &common::Rect,
        reco_detail: &str,
    ) -> MaaResult<i64> {
        let c_entry = CString::new(entry)?;
        let c_pipeline = CString::new(pipeline_override)?;
        let c_detail = CString::new(reco_detail)?;
        let maa_rect = sys::MaaRect {
            x: box_rect.x,
            y: box_rect.y,
            width: box_rect.width,
            height: box_rect.height,
        };
        let id = unsafe {
            sys::MaaContextRunAction(
                self.handle.as_ptr(),
                c_entry.as_ptr(),
                c_pipeline.as_ptr(),
                &maa_rect,
                c_detail.as_ptr(),
            )
        };
        Ok(id)
    }

    /// Override the next list for a node.
    ///
    /// # Arguments
    /// * `node_name` - Name of the node to modify
    /// * `next_list` - New next list, supports prefixes like `[JumpBack]` and `[Anchor]`
    ///
    /// Returns true if successful, false otherwise.
    pub fn override_next(&self, node_name: &str, next_list: &[&str]) -> MaaResult<bool> {
        let c_name = CString::new(node_name)?;
        let list_buf = crate::buffer::MaaStringListBuffer::new()?;
        for item in next_list {
            list_buf.append(item)?;
        }
        let ret = unsafe {
            sys::MaaContextOverrideNext(self.handle.as_ptr(), c_name.as_ptr(), list_buf.raw())
        };
        Ok(ret != 0)
    }

    pub fn get_node_data(&self, node_name: &str) -> MaaResult<Option<String>> {
        let c_name = CString::new(node_name)?;
        let buffer = crate::buffer::MaaStringBuffer::new()?;
        let ret = unsafe {
            sys::MaaContextGetNodeData(self.handle.as_ptr(), c_name.as_ptr(), buffer.raw())
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

    pub fn task_id(&self) -> common::MaaId {
        unsafe { sys::MaaContextGetTaskId(self.handle.as_ptr()) }
    }

    pub fn set_anchor(&self, anchor_name: &str, node_name: &str) -> MaaResult<()> {
        let c_anchor = CString::new(anchor_name)?;
        let c_node = CString::new(node_name)?;
        let ret = unsafe {
            sys::MaaContextSetAnchor(self.handle.as_ptr(), c_anchor.as_ptr(), c_node.as_ptr())
        };
        common::check_bool(ret)
    }

    pub fn get_anchor(&self, anchor_name: &str) -> MaaResult<Option<String>> {
        let c_anchor = CString::new(anchor_name)?;
        let buffer = crate::buffer::MaaStringBuffer::new()?;
        let ret = unsafe {
            sys::MaaContextGetAnchor(self.handle.as_ptr(), c_anchor.as_ptr(), buffer.raw())
        };
        if ret != 0 {
            Ok(Some(buffer.to_string()))
        } else {
            Ok(None)
        }
    }

    pub fn get_hit_count(&self, node_name: &str) -> MaaResult<u64> {
        let c_name = CString::new(node_name)?;
        let mut count: u64 = 0;
        let ret = unsafe {
            sys::MaaContextGetHitCount(self.handle.as_ptr(), c_name.as_ptr(), &mut count)
        };
        if ret != 0 {
            Ok(count)
        } else {
            Ok(0)
        }
    }

    pub fn clear_hit_count(&self, node_name: &str) -> MaaResult<()> {
        let c_name = CString::new(node_name)?;
        let ret = unsafe { sys::MaaContextClearHitCount(self.handle.as_ptr(), c_name.as_ptr()) };
        common::check_bool(ret)
    }

    /// Clone the context for independent execution.
    ///
    /// The cloned context can be used to run tasks independently without
    /// affecting the original context's state.
    pub fn clone_context(&self) -> MaaResult<Self> {
        let cloned = unsafe { sys::MaaContextClone(self.handle.as_ptr()) };
        NonNull::new(cloned)
            .map(|handle| Self {
                handle,
                callbacks: std::sync::Mutex::new(std::collections::HashMap::new()),
            })
            .ok_or(crate::MaaError::NullPointer)
    }

    /// Get the associated Tasker handle.
    ///
    /// Returns a raw pointer to the tasker. The caller should not destroy this handle
    /// as it is owned by the framework.
    pub fn tasker_handle(&self) -> *mut sys::MaaTasker {
        unsafe { sys::MaaContextGetTasker(self.handle.as_ptr()) }
    }

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
            sys::MaaContextOverrideImage(self.handle.as_ptr(), c_name.as_ptr(), image.raw())
        };
        common::check_bool(ret)
    }

    /// Get a job handle for the current task.
    ///
    /// Returns a `JobWithResult` that can be used to wait for and retrieve
    /// the task result.
    pub fn get_task_job(&self) -> crate::job::JobWithResult<common::TaskDetail> {
        let task_id = self.task_id();
        let tasker_ptr = crate::job::SendSyncPtr::new(self.tasker_handle());

        let status_fn: crate::job::StatusFn = Box::new(move |job_id| {
            common::MaaStatus(unsafe { sys::MaaTaskerStatus(tasker_ptr.get(), job_id) })
        });

        let wait_fn: crate::job::WaitFn = Box::new(move |job_id| {
            common::MaaStatus(unsafe { sys::MaaTaskerWait(tasker_ptr.get(), job_id) })
        });

        let get_ptr = crate::job::SendSyncPtr::new(self.tasker_handle());
        let get_fn = move |tid: common::MaaId| -> MaaResult<Option<common::TaskDetail>> {
            let entry_buf = crate::buffer::MaaStringBuffer::new()?;
            let mut node_ids = [0i64; 128];
            let mut node_count: u64 = 128;
            let mut status: i32 = 0;

            let ret = unsafe {
                sys::MaaTaskerGetTaskDetail(
                    get_ptr.get(),
                    tid,
                    entry_buf.raw(),
                    node_ids.as_mut_ptr(),
                    &mut node_count,
                    &mut status,
                )
            };

            if ret == 0 {
                return Ok(None);
            }

            let node_id_list = node_ids[..node_count as usize].to_vec();
            Ok(Some(common::TaskDetail {
                entry: entry_buf.to_string(),
                node_id_list,
                status: common::MaaStatus(status),
            }))
        };

        crate::job::JobWithResult::new(task_id, status_fn, wait_fn, get_fn)
    }

    /// Clear hit counts for all nodes.
    ///
    /// This resets all node hit counters in this context to zero.
    pub fn clear_all_hit_counts(&self) -> MaaResult<()> {
        let ret = unsafe { sys::MaaContextClearHitCount(self.handle.as_ptr(), std::ptr::null()) };
        common::check_bool(ret)
    }
}

impl Drop for Context {
    fn drop(&mut self) {
        // Context is a borrowed reference - cleanup of callbacks is managed by Tasker
        // via add_context_sink/clear_context_sinks, not by Context itself.
        let mut callbacks = self.callbacks.lock().unwrap();
        for (_, ptr) in callbacks.drain() {
            unsafe { crate::callback::EventCallback::drop_callback(ptr) };
        }
    }
}
