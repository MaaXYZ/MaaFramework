//! Agent client for connecting to AgentServer.
//!
//! This module allows delegating custom recognition and action execution
//! to a separate process running an AgentServer.

use crate::{
    buffer, common, controller::Controller, resource::Resource, sys, tasker::Tasker, MaaError,
    MaaResult,
};
use std::ptr::NonNull;

/// Agent client for remote custom component execution.
///
/// Connects to an AgentServer to delegate custom recognition and action
/// execution to a separate process.
pub struct AgentClient {
    handle: NonNull<sys::MaaAgentClient>,
}

unsafe impl Send for AgentClient {}
unsafe impl Sync for AgentClient {}

impl AgentClient {
    /// Create a new agent client.
    ///
    /// # Arguments
    /// * `identifier` - Optional connection identifier for matching specific AgentServer
    pub fn new(identifier: Option<&str>) -> MaaResult<Self> {
        let id_buffer = if let Some(id) = identifier {
            let mut buf = buffer::MaaStringBuffer::new()?;
            buf.set(id)?;
            Some(buf)
        } else {
            None
        };

        let handle = unsafe {
            sys::MaaAgentClientCreateV2(
                id_buffer
                    .as_ref()
                    .map(|b| b.raw())
                    .unwrap_or(std::ptr::null_mut()),
            )
        };

        NonNull::new(handle)
            .map(|ptr| Self { handle: ptr })
            .ok_or(MaaError::FrameworkError(-1))
    }

    /// Get the connection identifier.
    pub fn identifier(&self) -> Option<String> {
        let buffer = buffer::MaaStringBuffer::new().ok()?;
        let ret = unsafe { sys::MaaAgentClientIdentifier(self.handle.as_ptr(), buffer.raw()) };
        if ret != 0 {
            Some(buffer.to_string())
        } else {
            None
        }
    }

    /// Bind a resource to receive custom recognitions and actions from AgentServer.
    pub fn bind(&self, resource: &Resource) -> MaaResult<()> {
        let ret = unsafe { sys::MaaAgentClientBindResource(self.handle.as_ptr(), resource.raw()) };
        common::check_bool(ret)
    }

    /// Register resource event sink to forward events to AgentServer.
    pub fn register_resource_sink(&self, resource: &Resource) -> MaaResult<()> {
        let ret = unsafe {
            sys::MaaAgentClientRegisterResourceSink(self.handle.as_ptr(), resource.raw())
        };
        common::check_bool(ret)
    }

    /// Register controller event sink to forward events to AgentServer.
    pub fn register_controller_sink(&self, controller: &Controller) -> MaaResult<()> {
        let ret = unsafe {
            sys::MaaAgentClientRegisterControllerSink(self.handle.as_ptr(), controller.raw())
        };
        common::check_bool(ret)
    }

    /// Register tasker event sink to forward events to AgentServer.
    pub fn register_tasker_sink(&self, tasker: &Tasker) -> MaaResult<()> {
        let ret =
            unsafe { sys::MaaAgentClientRegisterTaskerSink(self.handle.as_ptr(), tasker.raw()) };
        common::check_bool(ret)
    }

    /// Register all event sinks (resource, controller, tasker) at once.
    pub fn register_sinks(
        &self,
        resource: &Resource,
        controller: &Controller,
        tasker: &Tasker,
    ) -> MaaResult<()> {
        self.register_resource_sink(resource)?;
        self.register_controller_sink(controller)?;
        self.register_tasker_sink(tasker)
    }

    /// Connect to the AgentServer.
    pub fn connect(&self) -> MaaResult<()> {
        let ret = unsafe { sys::MaaAgentClientConnect(self.handle.as_ptr()) };
        common::check_bool(ret)
    }

    /// Disconnect from the AgentServer.
    pub fn disconnect(&self) -> MaaResult<()> {
        let ret = unsafe { sys::MaaAgentClientDisconnect(self.handle.as_ptr()) };
        common::check_bool(ret)
    }

    /// Check if currently connected to AgentServer.
    pub fn connected(&self) -> bool {
        unsafe { sys::MaaAgentClientConnected(self.handle.as_ptr()) != 0 }
    }

    /// Check if the connection is alive.
    pub fn alive(&self) -> bool {
        unsafe { sys::MaaAgentClientAlive(self.handle.as_ptr()) != 0 }
    }

    /// Set the connection timeout.
    ///
    /// # Arguments
    /// * `milliseconds` - Timeout in milliseconds
    pub fn set_timeout(&self, milliseconds: i64) -> MaaResult<()> {
        let ret = unsafe { sys::MaaAgentClientSetTimeout(self.handle.as_ptr(), milliseconds) };
        common::check_bool(ret)
    }

    /// Get the list of custom recognitions available on the AgentServer.
    pub fn custom_recognition_list(&self) -> MaaResult<Vec<String>> {
        let buffer = buffer::MaaStringListBuffer::new()?;
        let ret = unsafe {
            sys::MaaAgentClientGetCustomRecognitionList(self.handle.as_ptr(), buffer.raw())
        };
        if ret != 0 {
            Ok(buffer.to_vec())
        } else {
            Err(MaaError::FrameworkError(0))
        }
    }

    /// Get the list of custom actions available on the AgentServer.
    pub fn custom_action_list(&self) -> MaaResult<Vec<String>> {
        let buffer = buffer::MaaStringListBuffer::new()?;
        let ret =
            unsafe { sys::MaaAgentClientGetCustomActionList(self.handle.as_ptr(), buffer.raw()) };
        if ret != 0 {
            Ok(buffer.to_vec())
        } else {
            Err(MaaError::FrameworkError(0))
        }
    }

    /// Get the raw handle pointer.
    pub fn raw(&self) -> *mut sys::MaaAgentClient {
        self.handle.as_ptr()
    }
}

impl Drop for AgentClient {
    fn drop(&mut self) {
        unsafe { sys::MaaAgentClientDestroy(self.handle.as_ptr()) }
    }
}
