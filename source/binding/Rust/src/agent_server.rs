//! Agent server for hosting custom components.
//!
//! This module allows hosting custom recognition and action components
//! in a separate process, which can be connected to by AgentClient.

use crate::{callback, common, sys, MaaError, MaaResult};
use std::ffi::CString;

/// Static functions for AgentServer management.
///
/// AgentServer hosts custom recognitions and actions that can be
/// accessed remotely by AgentClient instances.
pub struct AgentServer;

impl AgentServer {
    /// Register a custom recognition with the AgentServer.
    ///
    /// The recognition will be available to connected AgentClients.
    pub fn register_custom_recognition(
        name: &str,
        reco: Box<dyn crate::custom::CustomRecognition>,
    ) -> MaaResult<()> {
        let c_name = CString::new(name)?;
        let reco_ptr = Box::into_raw(Box::new(reco));
        let reco_ptr_void = reco_ptr as *mut std::ffi::c_void;

        unsafe {
            let ret = sys::MaaAgentServerRegisterCustomRecognition(
                c_name.as_ptr(),
                Some(crate::custom::custom_recognition_trampoline),
                reco_ptr_void,
            );
            if ret == 0 {
                let _ = Box::from_raw(reco_ptr);
                return Err(MaaError::FrameworkError(0));
            }
        }

        Ok(())
    }

    /// Register a custom action with the AgentServer.
    ///
    /// The action will be available to connected AgentClients.
    pub fn register_custom_action(
        name: &str,
        action: Box<dyn crate::custom::CustomAction>,
    ) -> MaaResult<()> {
        let c_name = CString::new(name)?;
        let action_ptr = Box::into_raw(Box::new(action));
        let action_ptr_void = action_ptr as *mut std::ffi::c_void;

        unsafe {
            let ret = sys::MaaAgentServerRegisterCustomAction(
                c_name.as_ptr(),
                Some(crate::custom::custom_action_trampoline),
                action_ptr_void,
            );
            if ret == 0 {
                let _ = Box::from_raw(action_ptr);
                return Err(MaaError::FrameworkError(0));
            }
        }
        Ok(())
    }

    /// Add a resource event sink.
    pub fn add_resource_sink<F>(callback: F) -> MaaResult<sys::MaaSinkId>
    where
        F: Fn(&str, &str) + Send + Sync + 'static,
    {
        let (cb, arg) = callback::EventCallback::new(callback);
        let sink_id = unsafe { sys::MaaAgentServerAddResourceSink(cb, arg) };
        if sink_id != 0 {
            Ok(sink_id)
        } else {
            unsafe { callback::EventCallback::drop_callback(arg) };
            Err(MaaError::FrameworkError(0))
        }
    }

    /// Add a controller event sink.
    pub fn add_controller_sink<F>(callback: F) -> MaaResult<sys::MaaSinkId>
    where
        F: Fn(&str, &str) + Send + Sync + 'static,
    {
        let (cb, arg) = callback::EventCallback::new(callback);
        let sink_id = unsafe { sys::MaaAgentServerAddControllerSink(cb, arg) };
        if sink_id != 0 {
            Ok(sink_id)
        } else {
            unsafe { callback::EventCallback::drop_callback(arg) };
            Err(MaaError::FrameworkError(0))
        }
    }

    /// Add a tasker event sink.
    pub fn add_tasker_sink<F>(callback: F) -> MaaResult<sys::MaaSinkId>
    where
        F: Fn(&str, &str) + Send + Sync + 'static,
    {
        let (cb, arg) = callback::EventCallback::new(callback);
        let sink_id = unsafe { sys::MaaAgentServerAddTaskerSink(cb, arg) };
        if sink_id != 0 {
            Ok(sink_id)
        } else {
            unsafe { callback::EventCallback::drop_callback(arg) };
            Err(MaaError::FrameworkError(0))
        }
    }

    /// Add a context event sink.
    pub fn add_context_sink<F>(callback: F) -> MaaResult<sys::MaaSinkId>
    where
        F: Fn(&str, &str) + Send + Sync + 'static,
    {
        let (cb, arg) = callback::EventCallback::new(callback);
        let sink_id = unsafe { sys::MaaAgentServerAddContextSink(cb, arg) };
        if sink_id != 0 {
            Ok(sink_id)
        } else {
            unsafe { callback::EventCallback::drop_callback(arg) };
            Err(MaaError::FrameworkError(0))
        }
    }

    /// Start the AgentServer.
    ///
    /// # Arguments
    /// * `identifier` - Connection identifier for clients to connect to
    pub fn start_up(identifier: &str) -> MaaResult<()> {
        let c_id = CString::new(identifier)?;
        let ret = unsafe { sys::MaaAgentServerStartUp(c_id.as_ptr()) };
        common::check_bool(ret)
    }

    /// Shut down the AgentServer.
    pub fn shut_down() {
        unsafe { sys::MaaAgentServerShutDown() }
    }

    /// Block until the server shuts down.
    pub fn join() {
        unsafe { sys::MaaAgentServerJoin() }
    }

    /// Detach the server to run in background.
    pub fn detach() {
        unsafe { sys::MaaAgentServerDetach() }
    }
}
