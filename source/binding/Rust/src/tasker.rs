use crate::buffer::{ImageBuffer, ImageListBuffer, RectBuffer, StringBuffer};
use crate::controller::Controller;
use crate::define::*;
use crate::error::{MaaError, Result};
use crate::ffi;
use crate::job::TaskJob;
use crate::resource::Resource;
use serde_json::Value;
use std::ffi::CString;

#[derive(Debug, Clone)]
pub struct RecognitionDetail {
    pub reco_id: MaaRecoId,
    pub name: String,
    pub algorithm: Algorithm,
    pub hit: bool,
    pub box_: Option<Rect>,
    pub raw_detail: Value,
    pub raw_image: Vec<u8>,
    pub draw_images: Vec<Vec<u8>>,
}

#[derive(Debug, Clone)]
pub struct ActionDetail {
    pub action_id: MaaActId,
    pub name: String,
    pub action: Action,
    pub box_: Rect,
    pub success: bool,
    pub raw_detail: Value,
}

#[derive(Debug, Clone)]
pub struct NodeDetail {
    pub node_id: MaaNodeId,
    pub name: String,
    pub recognition: Option<RecognitionDetail>,
    pub action: Option<ActionDetail>,
    pub completed: bool,
}

#[derive(Debug, Clone)]
pub struct TaskDetail {
    pub task_id: MaaTaskId,
    pub entry: String,
    pub nodes: Vec<NodeDetail>,
    pub status: Status,
}

pub struct Tasker {
    handle: MaaTaskerHandle,
    own: bool,
    _resource_holder: Option<Resource>,
    _controller_holder: Option<Controller>,
}

impl Tasker {
    pub fn new() -> Result<Self> {
        let handle = ffi::maa_tasker_create();
        if handle.is_null() {
            return Err(MaaError::CreateFailed("Tasker"));
        }
        Ok(Self {
            handle,
            own: true,
            _resource_holder: None,
            _controller_holder: None,
        })
    }

    pub(crate) fn from_handle(handle: MaaTaskerHandle) -> Self {
        Self {
            handle,
            own: false,
            _resource_holder: None,
            _controller_holder: None,
        }
    }

    pub fn handle(&self) -> MaaTaskerHandle {
        self.handle
    }

    pub fn bind(&mut self, resource: Resource, controller: Controller) -> bool {
        let res_ret = ffi::maa_tasker_bind_resource(self.handle, resource.handle());
        let ctrl_ret = ffi::maa_tasker_bind_controller(self.handle, controller.handle());
        self._resource_holder = Some(resource);
        self._controller_holder = Some(controller);
        res_ret != 0 && ctrl_ret != 0
    }

    pub fn bind_resource(&mut self, resource: Resource) -> bool {
        let ret = ffi::maa_tasker_bind_resource(self.handle, resource.handle());
        self._resource_holder = Some(resource);
        ret != 0
    }

    pub fn bind_controller(&mut self, controller: Controller) -> bool {
        let ret = ffi::maa_tasker_bind_controller(self.handle, controller.handle());
        self._controller_holder = Some(controller);
        ret != 0
    }

    pub fn resource(&self) -> Option<Resource> {
        let handle = ffi::maa_tasker_get_resource(self.handle);
        if handle.is_null() {
            None
        } else {
            Some(Resource::from_handle(handle))
        }
    }

    pub fn controller(&self) -> Option<Controller> {
        let handle = ffi::maa_tasker_get_controller(self.handle);
        if handle.is_null() {
            None
        } else {
            Some(Controller::from_handle(handle))
        }
    }

    pub fn inited(&self) -> bool {
        ffi::maa_tasker_inited(self.handle) != 0
    }

    pub fn post_task(&self, entry: &str, pipeline_override: &Value) -> Result<TaskJob> {
        let entry_cstr = CString::new(entry).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let override_str = serde_json::to_string(pipeline_override)?;
        let override_cstr =
            CString::new(override_str).map_err(|e| MaaError::BufferError(e.to_string()))?;

        let id =
            ffi::maa_tasker_post_task(self.handle, entry_cstr.as_ptr(), override_cstr.as_ptr());
        Ok(TaskJob::new(id, self.handle))
    }

    pub fn running(&self) -> bool {
        ffi::maa_tasker_running(self.handle) != 0
    }

    pub fn post_stop(&self) -> TaskJob {
        let id = ffi::maa_tasker_post_stop(self.handle);
        TaskJob::new(id, self.handle)
    }

    pub fn stopping(&self) -> bool {
        ffi::maa_tasker_stopping(self.handle) != 0
    }

    pub fn get_latest_node(&self, name: &str) -> Result<Option<NodeDetail>> {
        let name_cstr = CString::new(name).map_err(|e| MaaError::BufferError(e.to_string()))?;
        let mut node_id: MaaNodeId = 0;

        if ffi::maa_tasker_get_latest_node(self.handle, name_cstr.as_ptr(), &mut node_id) == 0 {
            return Ok(None);
        }

        self.get_node_detail(node_id)
    }

    pub fn clear_cache(&self) -> bool {
        ffi::maa_tasker_clear_cache(self.handle) != 0
    }

    pub fn get_recognition_detail(&self, reco_id: MaaRecoId) -> Result<Option<RecognitionDetail>> {
        let name_buf = StringBuffer::new();
        let algorithm_buf = StringBuffer::new();
        let mut hit: MaaBool = 0;
        let box_buf = RectBuffer::new();
        let detail_buf = StringBuffer::new();
        let raw_buf = ImageBuffer::new();
        let draws_buf = ImageListBuffer::new();

        if ffi::maa_tasker_get_recognition_detail(
            self.handle,
            reco_id,
            name_buf.handle(),
            algorithm_buf.handle(),
            &mut hit,
            box_buf.handle(),
            detail_buf.handle(),
            raw_buf.handle(),
            draws_buf.handle(),
        ) == 0
        {
            return Ok(None);
        }

        let name = name_buf.get()?;
        let algorithm_str = algorithm_buf.get()?;
        let algorithm = Algorithm::from(algorithm_str.as_str());
        let detail_str = detail_buf.get()?;
        let raw_detail: Value = if detail_str.is_empty() {
            Value::Null
        } else {
            serde_json::from_str(&detail_str)?
        };

        let box_ = if hit != 0 {
            Some(box_buf.get())
        } else {
            None
        };

        let raw_image = raw_buf.to_vec();
        let draws = draws_buf.get();
        let draw_images: Vec<Vec<u8>> = draws.iter().map(|img| img.to_vec()).collect();

        Ok(Some(RecognitionDetail {
            reco_id,
            name,
            algorithm,
            hit: hit != 0,
            box_,
            raw_detail,
            raw_image,
            draw_images,
        }))
    }

    pub fn get_action_detail(&self, action_id: MaaActId) -> Result<Option<ActionDetail>> {
        let name_buf = StringBuffer::new();
        let action_buf = StringBuffer::new();
        let box_buf = RectBuffer::new();
        let mut success: MaaBool = 0;
        let detail_buf = StringBuffer::new();

        if ffi::maa_tasker_get_action_detail(
            self.handle,
            action_id,
            name_buf.handle(),
            action_buf.handle(),
            box_buf.handle(),
            &mut success,
            detail_buf.handle(),
        ) == 0
        {
            return Ok(None);
        }

        let name = name_buf.get()?;
        let action_str = action_buf.get()?;
        let action = Action::from(action_str.as_str());
        let detail_str = detail_buf.get()?;
        let raw_detail: Value = if detail_str.is_empty() {
            Value::Null
        } else {
            serde_json::from_str(&detail_str)?
        };

        Ok(Some(ActionDetail {
            action_id,
            name,
            action,
            box_: box_buf.get(),
            success: success != 0,
            raw_detail,
        }))
    }

    pub fn get_node_detail(&self, node_id: MaaNodeId) -> Result<Option<NodeDetail>> {
        let name_buf = StringBuffer::new();
        let mut reco_id: MaaRecoId = 0;
        let mut action_id: MaaActId = 0;
        let mut completed: MaaBool = 0;

        if ffi::maa_tasker_get_node_detail(
            self.handle,
            node_id,
            name_buf.handle(),
            &mut reco_id,
            &mut action_id,
            &mut completed,
        ) == 0
        {
            return Ok(None);
        }

        let name = name_buf.get()?;
        let recognition = if reco_id != 0 {
            self.get_recognition_detail(reco_id)?
        } else {
            None
        };
        let action = if action_id != 0 {
            self.get_action_detail(action_id)?
        } else {
            None
        };

        Ok(Some(NodeDetail {
            node_id,
            name,
            recognition,
            action,
            completed: completed != 0,
        }))
    }

    pub fn get_task_detail(&self, task_id: MaaTaskId) -> Result<Option<TaskDetail>> {
        let entry_buf = StringBuffer::new();
        let mut size: MaaSize = 0;
        let mut status: MaaStatus = 0;

        if ffi::maa_tasker_get_task_detail(
            self.handle,
            task_id,
            entry_buf.handle(),
            std::ptr::null_mut(),
            &mut size,
            &mut status,
        ) == 0
        {
            return Ok(None);
        }

        let mut node_ids: Vec<MaaNodeId> = vec![0; size as usize];
        if ffi::maa_tasker_get_task_detail(
            self.handle,
            task_id,
            entry_buf.handle(),
            node_ids.as_mut_ptr(),
            &mut size,
            &mut status,
        ) == 0
        {
            return Ok(None);
        }

        let entry = entry_buf.get()?;
        let mut nodes = Vec::new();
        for &node_id in &node_ids {
            if let Some(detail) = self.get_node_detail(node_id)? {
                nodes.push(detail);
            }
        }

        Ok(Some(TaskDetail {
            task_id,
            entry,
            nodes,
            status: Status::from(status),
        }))
    }
}

impl Drop for Tasker {
    fn drop(&mut self) {
        if self.own && !self.handle.is_null() {
            ffi::maa_tasker_destroy(self.handle);
        }
    }
}

unsafe impl Send for Tasker {}
unsafe impl Sync for Tasker {}

// Global options
pub fn set_log_dir(path: impl AsRef<std::path::Path>) -> bool {
    let path_str = path.as_ref().to_string_lossy();
    let cstr = CString::new(path_str.as_ref()).unwrap();
    ffi::maa_global_set_option(
        GlobalOption::LogDir as i32,
        cstr.as_ptr() as *mut std::ffi::c_void,
        path_str.len() as u64,
    ) != 0
}

pub fn set_save_draw(save: bool) -> bool {
    let val: u8 = if save { 1 } else { 0 };
    ffi::maa_global_set_option(
        GlobalOption::SaveDraw as i32,
        &val as *const u8 as *mut std::ffi::c_void,
        std::mem::size_of::<u8>() as u64,
    ) != 0
}

pub fn set_stdout_level(level: LoggingLevel) -> bool {
    let val = level as i32;
    ffi::maa_global_set_option(
        GlobalOption::StdoutLevel as i32,
        &val as *const i32 as *mut std::ffi::c_void,
        std::mem::size_of::<i32>() as u64,
    ) != 0
}

pub fn set_debug_mode(debug: bool) -> bool {
    let val: u8 = if debug { 1 } else { 0 };
    ffi::maa_global_set_option(
        GlobalOption::DebugMode as i32,
        &val as *const u8 as *mut std::ffi::c_void,
        std::mem::size_of::<u8>() as u64,
    ) != 0
}

pub fn load_plugin(path: impl AsRef<std::path::Path>) -> bool {
    let path_str = path.as_ref().to_string_lossy();
    let cstr = CString::new(path_str.as_ref()).unwrap();
    ffi::maa_global_load_plugin(cstr.as_ptr()) != 0
}

pub fn version() -> String {
    let ptr = ffi::maa_version();
    if ptr.is_null() {
        return String::new();
    }
    let cstr = unsafe { std::ffi::CStr::from_ptr(ptr) };
    cstr.to_string_lossy().into_owned()
}

