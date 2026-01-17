use crate::resource::Resource;
use crate::{common, sys, MaaError, MaaResult};
use std::ptr::NonNull;

use std::collections::HashMap;
use std::ffi::c_void;
use std::sync::Mutex;

/// Task manager for executing pipelines.
///
/// Tasker is the central component that coordinates:
/// - Resource binding (images, models)
/// - Controller binding (device connection)
/// - Task execution (pipelines)
/// - Event handling (callbacks)
pub struct Tasker {
    handle: NonNull<sys::MaaTasker>,
    callbacks: Mutex<HashMap<sys::MaaSinkId, usize>>, // Store pointer address
}

unsafe impl Send for Tasker {}
unsafe impl Sync for Tasker {}

impl Tasker {
    pub fn new() -> MaaResult<Self> {
        let handle = unsafe { sys::MaaTaskerCreate() };
        if let Some(ptr) = NonNull::new(handle) {
            Ok(Self {
                handle: ptr,
                callbacks: Mutex::new(HashMap::new()),
            })
        } else {
            Err(MaaError::NullPointer)
        }
    }

    pub fn bind_resource(&self, res: &Resource) -> MaaResult<()> {
        let ret = unsafe { sys::MaaTaskerBindResource(self.handle.as_ptr(), res.raw()) };
        common::check_bool(ret)
    }

    pub fn bind_controller(&self, ctrl: &crate::controller::Controller) -> MaaResult<()> {
        let ret = unsafe { sys::MaaTaskerBindController(self.handle.as_ptr(), ctrl.raw()) };
        common::check_bool(ret)
    }

    pub fn get_recognition_detail(
        &self,
        reco_id: crate::common::MaaId,
    ) -> MaaResult<Option<crate::common::RecognitionDetail>> {
        let node_name = crate::buffer::MaaStringBuffer::new()?;
        let algorithm = crate::buffer::MaaStringBuffer::new()?;
        let mut hit = 0;
        let mut box_rect = sys::MaaRect {
            x: 0,
            y: 0,
            width: 0,
            height: 0,
        };
        let detail = crate::buffer::MaaStringBuffer::new()?;
        let raw = crate::buffer::MaaImageBuffer::new()?;
        let draws = crate::buffer::MaaImageListBuffer::new()?;

        let ret = unsafe {
            sys::MaaTaskerGetRecognitionDetail(
                self.handle.as_ptr(),
                reco_id,
                node_name.raw(),
                algorithm.raw(),
                &mut hit,
                &mut box_rect,
                detail.raw(),
                raw.raw(),
                draws.raw(),
            )
        };

        if ret == 0 {
            return Ok(None);
        }

        Ok(Some(crate::common::RecognitionDetail {
            node_name: node_name.as_str().to_string(),
            algorithm: algorithm.as_str().to_string(),
            hit: hit != 0,
            box_rect: crate::common::Rect::from(box_rect),
            detail: serde_json::from_str(detail.as_str()).unwrap_or(serde_json::Value::Null),
            raw_image: raw.to_vec(),
            draw_images: draws.to_vec_of_vec(),
        }))
    }

    pub fn get_action_detail(
        &self,
        act_id: crate::common::MaaId,
    ) -> MaaResult<Option<crate::common::ActionDetail>> {
        let node_name = crate::buffer::MaaStringBuffer::new()?;
        let action = crate::buffer::MaaStringBuffer::new()?;
        let mut box_rect = sys::MaaRect {
            x: 0,
            y: 0,
            width: 0,
            height: 0,
        };
        let mut success = 0;
        let detail = crate::buffer::MaaStringBuffer::new()?;

        let ret = unsafe {
            sys::MaaTaskerGetActionDetail(
                self.handle.as_ptr(),
                act_id,
                node_name.raw(),
                action.raw(),
                &mut box_rect,
                &mut success,
                detail.raw(),
            )
        };

        if ret == 0 {
            return Ok(None);
        }

        Ok(Some(crate::common::ActionDetail {
            node_name: node_name.as_str().to_string(),
            action: action.as_str().to_string(),
            box_rect: crate::common::Rect::from(box_rect),
            success: success != 0,
            detail: serde_json::from_str(detail.as_str()).unwrap_or(serde_json::Value::Null),
        }))
    }

    pub fn get_node_detail(
        &self,
        node_id: crate::common::MaaId,
    ) -> MaaResult<Option<crate::common::NodeDetail>> {
        let node_name = crate::buffer::MaaStringBuffer::new()?;
        let mut reco_id = 0;
        let mut act_id = 0;
        let mut completed = 0;

        let ret = unsafe {
            sys::MaaTaskerGetNodeDetail(
                self.handle.as_ptr(),
                node_id,
                node_name.raw(),
                &mut reco_id,
                &mut act_id,
                &mut completed,
            )
        };

        if ret == 0 {
            return Ok(None);
        }

        Ok(Some(crate::common::NodeDetail {
            node_name: node_name.as_str().to_string(),
            reco_id,
            act_id,
            completed: completed != 0,
        }))
    }

    pub fn get_task_detail(
        &self,
        task_id: crate::common::MaaId,
    ) -> MaaResult<Option<crate::common::TaskDetail>> {
        let entry = crate::buffer::MaaStringBuffer::new()?;
        let mut node_id_list_size: sys::MaaSize = 0;
        let mut status: sys::MaaStatus = 0;

        let ret = unsafe {
            sys::MaaTaskerGetTaskDetail(
                self.handle.as_ptr(),
                task_id,
                entry.raw(),
                std::ptr::null_mut(),
                &mut node_id_list_size,
                &mut status,
            )
        };

        if ret == 0 {
            return Ok(None);
        }

        let mut node_id_list = vec![0; node_id_list_size as usize];
        let ret = unsafe {
            sys::MaaTaskerGetTaskDetail(
                self.handle.as_ptr(),
                task_id,
                entry.raw(),
                node_id_list.as_mut_ptr(),
                &mut node_id_list_size,
                &mut status,
            )
        };

        if ret == 0 {
            return Ok(None);
        }

        Ok(Some(crate::common::TaskDetail {
            entry: entry.as_str().to_string(),
            node_id_list,
            status: crate::common::MaaStatus(status as i32),
        }))
    }

    pub fn post_task(
        &self,
        entry: &str,
        pipeline_override: &str,
    ) -> MaaResult<crate::job::JobWithResult<crate::common::TaskDetail>> {
        let c_entry = std::ffi::CString::new(entry)?;
        let c_pipeline = std::ffi::CString::new(pipeline_override)?;
        let id = unsafe {
            sys::MaaTaskerPostTask(self.handle.as_ptr(), c_entry.as_ptr(), c_pipeline.as_ptr())
        };

        let ptr = crate::job::tasker_ptr(self.handle.as_ptr());
        let status_fn: crate::job::StatusFn = Box::new(move |job_id| {
            crate::common::MaaStatus(unsafe { sys::MaaTaskerStatus(ptr.get(), job_id) })
        });
        let wait_fn: crate::job::WaitFn = Box::new(move |job_id| {
            crate::common::MaaStatus(unsafe { sys::MaaTaskerWait(ptr.get(), job_id) })
        });

        let tasker_ptr = crate::job::tasker_ptr(self.handle.as_ptr());
        let get_fn =
            move |task_id: crate::common::MaaId| -> MaaResult<Option<crate::common::TaskDetail>> {
                let entry_buf = crate::buffer::MaaStringBuffer::new()?;
                let mut node_ids = [0i64; 128];
                let mut node_count: u64 = 128;
                let mut status: i32 = 0;

                let ret = unsafe {
                    sys::MaaTaskerGetTaskDetail(
                        tasker_ptr.get(),
                        task_id,
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
                Ok(Some(crate::common::TaskDetail {
                    entry: entry_buf.to_string(),
                    node_id_list,
                    status: crate::common::MaaStatus(status),
                }))
            };

        Ok(crate::job::JobWithResult::new(
            id, status_fn, wait_fn, get_fn,
        ))
    }

    /// Post a task with JSON pipeline override.
    ///
    /// Convenience method that accepts `serde_json::Value` for pipeline overrides.
    ///
    /// # Example
    /// ```ignore
    /// use serde_json::json;
    /// let job = tasker.post_task_json("StartTask", &json!({
    ///     "StartTask": { "next": ["SecondTask"] }
    /// }))?;
    /// ```
    pub fn post_task_json(
        &self,
        entry: &str,
        pipeline_override: &serde_json::Value,
    ) -> MaaResult<crate::job::JobWithResult<crate::common::TaskDetail>> {
        self.post_task(entry, &pipeline_override.to_string())
    }

    pub fn inited(&self) -> bool {
        unsafe { sys::MaaTaskerInited(self.handle.as_ptr()) != 0 }
    }

    pub fn raw(&self) -> *mut sys::MaaTasker {
        self.handle.as_ptr()
    }

    /// Add a tasker event sink callback.
    ///
    /// This registers a callback that will be invoked for all tasker events
    /// including task start, task completion, and status changes.
    ///
    /// # Arguments
    /// * `callback` - Closure that receives (message, detail_json) for each event
    ///
    /// # Returns
    /// Sink ID for later removal via `remove_sink()`
    pub fn add_sink<F>(&self, callback: F) -> MaaResult<sys::MaaSinkId>
    where
        F: Fn(&str, &str) + Send + Sync + 'static,
    {
        let (cb, arg) = crate::callback::EventCallback::new(callback);
        let id = unsafe { sys::MaaTaskerAddSink(self.handle.as_ptr(), cb, arg) };
        if id > 0 {
            self.callbacks.lock().unwrap().insert(id, arg as usize);
            Ok(id)
        } else {
            unsafe { crate::callback::EventCallback::drop_callback(arg) };
            Err(MaaError::FrameworkError(0))
        }
    }

    /// Remove a tasker sink by ID.
    ///
    /// # Arguments
    /// * `sink_id` - ID returned from `add_sink()`
    pub fn remove_sink(&self, sink_id: sys::MaaSinkId) {
        unsafe { sys::MaaTaskerRemoveSink(self.handle.as_ptr(), sink_id) };
        if let Some(ptr) = self.callbacks.lock().unwrap().remove(&sink_id) {
            unsafe { crate::callback::EventCallback::drop_callback(ptr as *mut c_void) };
        }
    }

    /// Clear all tasker sinks.
    pub fn clear_sinks(&self) {
        unsafe { sys::MaaTaskerClearSinks(self.handle.as_ptr()) };
        let mut callbacks = self.callbacks.lock().unwrap();
        for (_, ptr) in callbacks.drain() {
            unsafe { crate::callback::EventCallback::drop_callback(ptr as *mut c_void) };
        }
    }

    #[deprecated(since = "0.5.1", note = "Use add_sink() instead")]
    pub fn register_callback<F>(&self, callback: F) -> MaaResult<crate::common::MaaId>
    where
        F: Fn(&str, &str) + Send + Sync + 'static,
    {
        self.add_sink(callback)
    }

    pub fn post_stop(&self) -> MaaResult<crate::common::MaaId> {
        unsafe {
            let id = sys::MaaTaskerPostStop(self.handle.as_ptr());
            Ok(id)
        }
    }

    /// Check if the tasker is currently running.
    pub fn is_running(&self) -> bool {
        unsafe { sys::MaaTaskerRunning(self.handle.as_ptr()) != 0 }
    }

    /// Check if the tasker is currently running (alias for `is_running`).
    pub fn running(&self) -> bool {
        self.is_running()
    }

    /// Check if the tasker is currently stopping.
    pub fn stopping(&self) -> bool {
        unsafe { sys::MaaTaskerStopping(self.handle.as_ptr()) != 0 }
    }

    // === Context Sink ===

    /// Add a context event sink callback.
    /// Returns a sink ID for later removal.
    pub fn add_context_sink<F>(&self, callback: F) -> MaaResult<sys::MaaSinkId>
    where
        F: Fn(&str, &str) + Send + Sync + 'static,
    {
        let (cb, arg) = crate::callback::EventCallback::new(callback);
        let id = unsafe { sys::MaaTaskerAddContextSink(self.handle.as_ptr(), cb, arg) };
        if id > 0 {
            self.callbacks.lock().unwrap().insert(id, arg as usize);
            Ok(id)
        } else {
            unsafe { crate::callback::EventCallback::drop_callback(arg) };
            Err(MaaError::FrameworkError(0))
        }
    }

    /// Remove a context sink by ID.
    pub fn remove_context_sink(&self, sink_id: sys::MaaSinkId) {
        unsafe { sys::MaaTaskerRemoveContextSink(self.handle.as_ptr(), sink_id) };
        if let Some(ptr) = self.callbacks.lock().unwrap().remove(&sink_id) {
            unsafe { crate::callback::EventCallback::drop_callback(ptr as *mut c_void) };
        }
    }

    /// Clear all context sinks.
    pub fn clear_context_sinks(&self) {
        unsafe { sys::MaaTaskerClearContextSinks(self.handle.as_ptr()) };
        // Note: callbacks registered via add_context_sink will be cleaned up
    }

    pub fn clear_cache(&self) -> MaaResult<()> {
        let ret = unsafe { sys::MaaTaskerClearCache(self.handle.as_ptr()) };
        crate::common::check_bool(ret)
    }

    pub fn get_latest_node(&self, node_name: &str) -> MaaResult<Option<crate::common::MaaId>> {
        let c_name = std::ffi::CString::new(node_name)?;
        let mut node_id: crate::common::MaaId = 0;
        let ret = unsafe {
            sys::MaaTaskerGetLatestNode(self.handle.as_ptr(), c_name.as_ptr(), &mut node_id)
        };
        if ret != 0 && node_id != 0 {
            Ok(Some(node_id))
        } else {
            Ok(None)
        }
    }

    /// Convenience method to bind both resource and controller at once.
    pub fn bind(
        &self,
        resource: &Resource,
        controller: &crate::controller::Controller,
    ) -> MaaResult<()> {
        self.bind_resource(resource)?;
        self.bind_controller(controller)
    }

    /// Get a borrowed view of the bound resource.
    ///
    /// Returns `None` if no resource is bound.
    ///
    /// # Example
    /// ```ignore
    /// if let Some(res) = tasker.resource() {
    ///     println!("Loaded: {}", res.loaded());
    /// }
    /// ```
    pub fn resource(&self) -> Option<crate::resource::ResourceRef<'_>> {
        let ptr = unsafe { sys::MaaTaskerGetResource(self.handle.as_ptr()) };
        crate::resource::ResourceRef::from_ptr(ptr)
    }

    /// Get a borrowed view of the bound controller.
    ///
    /// Returns `None` if no controller is bound.
    ///
    /// # Example
    /// ```ignore
    /// if let Some(ctrl) = tasker.controller() {
    ///     println!("Connected: {}", ctrl.connected());
    /// }
    /// ```
    pub fn controller(&self) -> Option<crate::controller::ControllerRef<'_>> {
        let ptr = unsafe { sys::MaaTaskerGetController(self.handle.as_ptr()) };
        crate::controller::ControllerRef::from_ptr(ptr)
    }

    /// Get the bound resource handle (raw pointer).
    ///
    /// Returns the raw pointer to the resource. The caller should not destroy this handle.
    pub fn resource_handle(&self) -> *mut sys::MaaResource {
        unsafe { sys::MaaTaskerGetResource(self.handle.as_ptr()) }
    }

    /// Get the bound controller handle (raw pointer).
    ///
    /// Returns the raw pointer to the controller. The caller should not destroy this handle.
    pub fn controller_handle(&self) -> *mut sys::MaaController {
        unsafe { sys::MaaTaskerGetController(self.handle.as_ptr()) }
    }

    /// Post a recognition task directly without executing through a pipeline.
    ///
    /// # Arguments
    /// * `reco_type` - Recognition type (e.g., "TemplateMatch", "OCR")
    /// * `reco_param` - Recognition parameters as JSON string
    /// * `image` - The image to perform recognition on
    pub fn post_recognition(
        &self,
        reco_type: &str,
        reco_param: &str,
        image: &crate::buffer::MaaImageBuffer,
    ) -> MaaResult<crate::job::JobWithResult<crate::common::RecognitionDetail>> {
        let c_type = std::ffi::CString::new(reco_type)?;
        let c_param = std::ffi::CString::new(reco_param)?;
        let id = unsafe {
            sys::MaaTaskerPostRecognition(
                self.handle.as_ptr(),
                c_type.as_ptr(),
                c_param.as_ptr(),
                image.raw(),
            )
        };

        let ptr = crate::job::tasker_ptr(self.handle.as_ptr());
        let status_fn: crate::job::StatusFn = Box::new(move |job_id| {
            common::MaaStatus(unsafe { sys::MaaTaskerStatus(ptr.get(), job_id) })
        });
        let wait_fn: crate::job::WaitFn = Box::new(move |job_id| {
            common::MaaStatus(unsafe { sys::MaaTaskerWait(ptr.get(), job_id) })
        });

        let tasker_ptr = crate::job::tasker_ptr(self.handle.as_ptr());
        let get_fn = move |reco_id: common::MaaId| -> MaaResult<Option<common::RecognitionDetail>> {
            let node_name = crate::buffer::MaaStringBuffer::new()?;
            let algorithm = crate::buffer::MaaStringBuffer::new()?;
            let mut hit = 0;
            let mut box_rect = sys::MaaRect {
                x: 0,
                y: 0,
                width: 0,
                height: 0,
            };
            let detail = crate::buffer::MaaStringBuffer::new()?;
            let raw = crate::buffer::MaaImageBuffer::new()?;
            let draws = crate::buffer::MaaImageListBuffer::new()?;

            let ret = unsafe {
                sys::MaaTaskerGetRecognitionDetail(
                    tasker_ptr.get(),
                    reco_id,
                    node_name.raw(),
                    algorithm.raw(),
                    &mut hit,
                    &mut box_rect,
                    detail.raw(),
                    raw.raw(),
                    draws.raw(),
                )
            };

            if ret == 0 {
                return Ok(None);
            }

            Ok(Some(common::RecognitionDetail {
                node_name: node_name.as_str().to_string(),
                algorithm: algorithm.as_str().to_string(),
                hit: hit != 0,
                box_rect: common::Rect::from(box_rect),
                detail: serde_json::from_str(detail.as_str()).unwrap_or(serde_json::Value::Null),
                raw_image: raw.to_vec(),
                draw_images: draws.to_vec_of_vec(),
            }))
        };

        Ok(crate::job::JobWithResult::new(
            id, status_fn, wait_fn, get_fn,
        ))
    }

    /// Post an action task directly without executing through a pipeline.
    ///
    /// # Arguments
    /// * `action_type` - Action type (e.g., "Click", "Swipe")  
    /// * `action_param` - Action parameters as JSON string
    /// * `box_rect` - The target rectangle for the action
    /// * `reco_detail` - Recognition detail from previous recognition (can be empty)
    pub fn post_action(
        &self,
        action_type: &str,
        action_param: &str,
        box_rect: &common::Rect,
        reco_detail: &str,
    ) -> MaaResult<crate::job::JobWithResult<crate::common::ActionDetail>> {
        let c_type = std::ffi::CString::new(action_type)?;
        let c_param = std::ffi::CString::new(action_param)?;
        let c_detail = std::ffi::CString::new(reco_detail)?;
        let maa_rect = sys::MaaRect {
            x: box_rect.x,
            y: box_rect.y,
            width: box_rect.width,
            height: box_rect.height,
        };

        let id = unsafe {
            sys::MaaTaskerPostAction(
                self.handle.as_ptr(),
                c_type.as_ptr(),
                c_param.as_ptr(),
                &maa_rect,
                c_detail.as_ptr(),
            )
        };

        let ptr = crate::job::tasker_ptr(self.handle.as_ptr());
        let status_fn: crate::job::StatusFn = Box::new(move |job_id| {
            common::MaaStatus(unsafe { sys::MaaTaskerStatus(ptr.get(), job_id) })
        });
        let wait_fn: crate::job::WaitFn = Box::new(move |job_id| {
            common::MaaStatus(unsafe { sys::MaaTaskerWait(ptr.get(), job_id) })
        });

        let tasker_ptr = crate::job::tasker_ptr(self.handle.as_ptr());
        let get_fn = move |act_id: common::MaaId| -> MaaResult<Option<common::ActionDetail>> {
            let node_name = crate::buffer::MaaStringBuffer::new()?;
            let action = crate::buffer::MaaStringBuffer::new()?;
            let mut result_box = sys::MaaRect {
                x: 0,
                y: 0,
                width: 0,
                height: 0,
            };
            let mut success = 0;
            let detail = crate::buffer::MaaStringBuffer::new()?;

            let ret = unsafe {
                sys::MaaTaskerGetActionDetail(
                    tasker_ptr.get(),
                    act_id,
                    node_name.raw(),
                    action.raw(),
                    &mut result_box,
                    &mut success,
                    detail.raw(),
                )
            };

            if ret == 0 {
                return Ok(None);
            }

            Ok(Some(common::ActionDetail {
                node_name: node_name.as_str().to_string(),
                action: action.as_str().to_string(),
                box_rect: common::Rect::from(result_box),
                success: success != 0,
                detail: serde_json::from_str(detail.as_str()).unwrap_or(serde_json::Value::Null),
            }))
        };

        Ok(crate::job::JobWithResult::new(
            id, status_fn, wait_fn, get_fn,
        ))
    }
}

impl Drop for Tasker {
    fn drop(&mut self) {
        unsafe {
            sys::MaaTaskerClearSinks(self.handle.as_ptr());
            let mut callbacks = self.callbacks.lock().unwrap();
            for (_, ptr) in callbacks.drain() {
                crate::callback::EventCallback::drop_callback(ptr as *mut c_void);
            }
            sys::MaaTaskerDestroy(self.handle.as_ptr())
        }
    }
}
