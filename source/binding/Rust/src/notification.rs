//! Structured notification parsing for event callbacks.
//!
//! This module provides typed structures and parsing utilities for the event
//! notifications from MaaFramework. Instead of manually parsing JSON strings,
//! use these helpers to work with strongly-typed event data.
//!
//! # Example
//!
//! ```ignore
//! use maa_framework::notification::{self, NotificationType, msg};
//!
//! tasker.add_sink(|message, details| {
//!     let noti_type = notification::parse_type(message);
//!     
//!     if message.starts_with("Resource.Loading") {
//!         if let Some(detail) = notification::parse_resource_loading(details) {
//!             println!("Resource {} loading: {:?}", detail.res_id, noti_type);
//!         }
//!     }
//! })?;
//! ```

use serde::{Deserialize, Serialize};
use serde_json::Value;

// === Notification Type ===

/// Type of notification event.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum NotificationType {
    /// Operation is starting
    Starting,
    /// Operation succeeded
    Succeeded,
    /// Operation failed
    Failed,
    /// Unknown notification type
    Unknown,
}

impl NotificationType {
    /// Check if this is a starting notification.
    pub fn is_starting(&self) -> bool {
        matches!(self, Self::Starting)
    }

    /// Check if this is a succeeded notification.
    pub fn is_succeeded(&self) -> bool {
        matches!(self, Self::Succeeded)
    }

    /// Check if this is a failed notification.
    pub fn is_failed(&self) -> bool {
        matches!(self, Self::Failed)
    }

    /// Check if the operation is complete (succeeded or failed).
    pub fn is_complete(&self) -> bool {
        matches!(self, Self::Succeeded | Self::Failed)
    }
}

impl From<&str> for NotificationType {
    fn from(s: &str) -> Self {
        if s.ends_with(".Starting") {
            NotificationType::Starting
        } else if s.ends_with(".Succeeded") {
            NotificationType::Succeeded
        } else if s.ends_with(".Failed") {
            NotificationType::Failed
        } else {
            NotificationType::Unknown
        }
    }
}

// === Event Detail Structures ===

/// Resource loading event detail.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ResourceLoadingDetail {
    /// Resource ID
    pub res_id: i64,
    /// Resource hash
    pub hash: String,
    /// Path being loaded
    pub path: String,
}

/// Controller action event detail.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ControllerActionDetail {
    /// Controller ID
    pub ctrl_id: i64,
    /// Device UUID
    pub uuid: String,
    /// Action name
    pub action: String,
    /// Action parameters
    #[serde(default)]
    pub param: Value,
}

/// Tasker task event detail.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TaskerTaskDetail {
    /// Task ID
    pub task_id: i64,
    /// Entry node name
    pub entry: String,
    /// Device UUID
    #[serde(default)]
    pub uuid: String,
    /// Resource hash
    #[serde(default)]
    pub hash: String,
}

/// Next list item for node traversal.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct NextListItem {
    /// Node name
    pub name: String,
    /// Whether to jump back after execution
    #[serde(default)]
    pub jump_back: bool,
    /// Whether this is an anchor node
    #[serde(default)]
    pub anchor: bool,
}

/// Node next list event detail.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct NodeNextListDetail {
    /// Task ID
    pub task_id: i64,
    /// Current node name
    pub name: String,
    /// List of next nodes
    #[serde(default)]
    pub list: Vec<NextListItem>,
    /// Focus configuration
    #[serde(default)]
    pub focus: Value,
}

/// Node recognition event detail.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct NodeRecognitionDetail {
    /// Task ID
    pub task_id: i64,
    /// Recognition ID
    pub reco_id: i64,
    /// Node name
    pub name: String,
    /// Focus configuration
    #[serde(default)]
    pub focus: Value,
}

/// Node action event detail.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct NodeActionDetail {
    /// Task ID
    pub task_id: i64,
    /// Action ID
    pub action_id: i64,
    /// Node name
    pub name: String,
    /// Focus configuration
    #[serde(default)]
    pub focus: Value,
}

/// Node pipeline node event detail.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct NodePipelineNodeDetail {
    /// Task ID
    pub task_id: i64,
    /// Node ID
    pub node_id: i64,
    /// Node name
    pub name: String,
    /// Focus configuration
    #[serde(default)]
    pub focus: Value,
}

// === Message Constants ===

/// Notification message constants.
pub mod msg {
    // Resource events
    pub const RESOURCE_LOADING_STARTING: &str = "Resource.Loading.Starting";
    pub const RESOURCE_LOADING_SUCCEEDED: &str = "Resource.Loading.Succeeded";
    pub const RESOURCE_LOADING_FAILED: &str = "Resource.Loading.Failed";

    // Controller events
    pub const CONTROLLER_ACTION_STARTING: &str = "Controller.Action.Starting";
    pub const CONTROLLER_ACTION_SUCCEEDED: &str = "Controller.Action.Succeeded";
    pub const CONTROLLER_ACTION_FAILED: &str = "Controller.Action.Failed";

    // Tasker events
    pub const TASKER_TASK_STARTING: &str = "Tasker.Task.Starting";
    pub const TASKER_TASK_SUCCEEDED: &str = "Tasker.Task.Succeeded";
    pub const TASKER_TASK_FAILED: &str = "Tasker.Task.Failed";

    // Node pipeline events
    pub const NODE_PIPELINE_NODE_STARTING: &str = "Node.PipelineNode.Starting";
    pub const NODE_PIPELINE_NODE_SUCCEEDED: &str = "Node.PipelineNode.Succeeded";
    pub const NODE_PIPELINE_NODE_FAILED: &str = "Node.PipelineNode.Failed";

    // Node recognition events
    pub const NODE_RECOGNITION_STARTING: &str = "Node.Recognition.Starting";
    pub const NODE_RECOGNITION_SUCCEEDED: &str = "Node.Recognition.Succeeded";
    pub const NODE_RECOGNITION_FAILED: &str = "Node.Recognition.Failed";

    // Node action events
    pub const NODE_ACTION_STARTING: &str = "Node.Action.Starting";
    pub const NODE_ACTION_SUCCEEDED: &str = "Node.Action.Succeeded";
    pub const NODE_ACTION_FAILED: &str = "Node.Action.Failed";

    // Node next list events
    pub const NODE_NEXT_LIST_STARTING: &str = "Node.NextList.Starting";
    pub const NODE_NEXT_LIST_SUCCEEDED: &str = "Node.NextList.Succeeded";
    pub const NODE_NEXT_LIST_FAILED: &str = "Node.NextList.Failed";
}

// === Parse Functions ===

/// Parse notification type from message string.
///
/// # Example
/// ```ignore
/// let noti_type = notification::parse_type("Resource.Loading.Succeeded");
/// assert_eq!(noti_type, NotificationType::Succeeded);
/// ```
pub fn parse_type(msg: &str) -> NotificationType {
    NotificationType::from(msg)
}

/// Parse resource loading event detail from JSON.
pub fn parse_resource_loading(details: &str) -> Option<ResourceLoadingDetail> {
    serde_json::from_str(details).ok()
}

/// Parse controller action event detail from JSON.
pub fn parse_controller_action(details: &str) -> Option<ControllerActionDetail> {
    serde_json::from_str(details).ok()
}

/// Parse tasker task event detail from JSON.
pub fn parse_tasker_task(details: &str) -> Option<TaskerTaskDetail> {
    serde_json::from_str(details).ok()
}

/// Parse node next list event detail from JSON.
pub fn parse_node_next_list(details: &str) -> Option<NodeNextListDetail> {
    serde_json::from_str(details).ok()
}

/// Parse node recognition event detail from JSON.
pub fn parse_node_recognition(details: &str) -> Option<NodeRecognitionDetail> {
    serde_json::from_str(details).ok()
}

/// Parse node action event detail from JSON.
pub fn parse_node_action(details: &str) -> Option<NodeActionDetail> {
    serde_json::from_str(details).ok()
}

/// Parse node pipeline node event detail from JSON.
pub fn parse_node_pipeline_node(details: &str) -> Option<NodePipelineNodeDetail> {
    serde_json::from_str(details).ok()
}

// === Event Sink Traits ===

/// Trait for handling resource events.
pub trait ResourceEventHandler: Send + Sync {
    /// Called when a resource loading event occurs.
    fn on_resource_loading(&self, _noti_type: NotificationType, _detail: ResourceLoadingDetail) {}

    /// Called when an unknown notification is received.
    fn on_unknown(&self, _msg: &str, _details: &Value) {}
}

/// Trait for handling controller events.
pub trait ControllerEventHandler: Send + Sync {
    /// Called when a controller action event occurs.
    fn on_controller_action(&self, _noti_type: NotificationType, _detail: ControllerActionDetail) {}

    /// Called when an unknown notification is received.
    fn on_unknown(&self, _msg: &str, _details: &Value) {}
}

/// Trait for handling tasker events.
pub trait TaskerEventHandler: Send + Sync {
    /// Called when a tasker task event occurs.
    fn on_tasker_task(&self, _noti_type: NotificationType, _detail: TaskerTaskDetail) {}

    /// Called when an unknown notification is received.
    fn on_unknown(&self, _msg: &str, _details: &Value) {}
}

/// Trait for handling context/node events.
pub trait ContextEventHandler: Send + Sync {
    /// Called when a node next list event occurs.
    fn on_node_next_list(&self, _noti_type: NotificationType, _detail: NodeNextListDetail) {}

    /// Called when a node recognition event occurs.
    fn on_node_recognition(&self, _noti_type: NotificationType, _detail: NodeRecognitionDetail) {}

    /// Called when a node action event occurs.
    fn on_node_action(&self, _noti_type: NotificationType, _detail: NodeActionDetail) {}

    /// Called when a node pipeline node event occurs.
    fn on_node_pipeline_node(&self, _noti_type: NotificationType, _detail: NodePipelineNodeDetail) {
    }

    /// Called when an unknown notification is received.
    fn on_unknown(&self, _msg: &str, _details: &Value) {}
}
