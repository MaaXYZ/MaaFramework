//! Event sink system for typed callback notifications.
//!
//! This module provides a trait-based event sink system that matches the Python SDK's
//! `EventSink` interface, allowing for structured handling of framework notifications.

use crate::common::{MaaId, NotificationType};
use serde::{Deserialize, Serialize};

/// Base trait for all event sinks.
///
/// Implement this trait to receive typed notifications from the framework.
/// Each notification comes with a message type and parsed detail structure.
pub trait EventSink: Send + Sync {
    /// Called for each event notification.
    ///
    /// # Arguments
    /// * `msg` - The message type (e.g., "Resource.Loading.Starting")
    /// * `details_json` - Raw JSON details string
    fn on_event(&self, msg: &str, details_json: &str);

    /// Called when the notification type cannot be determined.
    fn on_unknown(&self, _msg: &str, _details_json: &str) {}
}

// === Resource Events ===

/// Details for resource loading events.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ResourceLoadingDetail {
    pub res_id: MaaId,
    pub path: String,
    #[serde(rename = "type")]
    pub res_type: String,
    pub hash: String,
}

/// Trait for resource-specific event handling.
pub trait ResourceEventSink: EventSink {
    fn on_loading_starting(&self, _detail: ResourceLoadingDetail) {}
    fn on_loading_succeeded(&self, _detail: ResourceLoadingDetail) {}
    fn on_loading_failed(&self, _detail: ResourceLoadingDetail) {}
}

// === Controller Events ===

/// Details for controller action events.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ControllerActionDetail {
    pub ctrl_id: MaaId,
    pub uuid: String,
    pub action: String,
    #[serde(default)]
    pub param: serde_json::Value,
}

/// Trait for controller-specific event handling.
pub trait ControllerEventSink: EventSink {
    fn on_action_starting(&self, _detail: ControllerActionDetail) {}
    fn on_action_succeeded(&self, _detail: ControllerActionDetail) {}
    fn on_action_failed(&self, _detail: ControllerActionDetail) {}
}

// === Tasker Events ===

/// Details for tasker task events.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TaskerTaskDetail {
    pub task_id: MaaId,
    pub entry: String,
    pub uuid: String,
    pub hash: String,
}

/// Trait for tasker-specific event handling.
pub trait TaskerEventSink: EventSink {
    fn on_task_starting(&self, _detail: TaskerTaskDetail) {}
    fn on_task_succeeded(&self, _detail: TaskerTaskDetail) {}
    fn on_task_failed(&self, _detail: TaskerTaskDetail) {}
}

// === Node Events ===

/// Details for node events (pipeline, recognition, action).
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct NodeDetail {
    pub task_id: MaaId,
    #[serde(default)]
    pub node_id: MaaId,
    pub name: String,
    #[serde(default)]
    pub focus: serde_json::Value,
}

/// Details for recognition events.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct RecognitionEventDetail {
    pub task_id: MaaId,
    pub reco_id: MaaId,
    pub name: String,
    #[serde(default)]
    pub focus: serde_json::Value,
}

/// Details for action events.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ActionEventDetail {
    pub task_id: MaaId,
    pub action_id: MaaId,
    pub name: String,
    #[serde(default)]
    pub focus: serde_json::Value,
}

/// Trait for context/node-specific event handling.
pub trait ContextEventSink: EventSink {
    // Pipeline node events
    fn on_pipeline_starting(&self, _detail: NodeDetail) {}
    fn on_pipeline_succeeded(&self, _detail: NodeDetail) {}
    fn on_pipeline_failed(&self, _detail: NodeDetail) {}

    // Recognition node events
    fn on_recognition_starting(&self, _detail: RecognitionEventDetail) {}
    fn on_recognition_succeeded(&self, _detail: RecognitionEventDetail) {}
    fn on_recognition_failed(&self, _detail: RecognitionEventDetail) {}

    // Action node events
    fn on_action_starting(&self, _detail: ActionEventDetail) {}
    fn on_action_succeeded(&self, _detail: ActionEventDetail) {}
    fn on_action_failed(&self, _detail: ActionEventDetail) {}
}

// === Helper Functions ===

/// Parse a notification message to determine its type.
pub fn parse_notification_type(msg: &str) -> NotificationType {
    NotificationType::from_message(msg)
}

/// Parse a notification message to get its category (e.g., "Resource", "Controller").
pub fn parse_category(msg: &str) -> Option<&str> {
    msg.split('.').next()
}

/// Create an event sink adapter from a closure.
///
/// This is a convenience function to wrap a simple closure as an EventSink.
pub fn from_closure<F>(f: F) -> ClosureEventSink<F>
where
    F: Fn(&str, &str) + Send + Sync,
{
    ClosureEventSink(f)
}

/// Wrapper type that implements EventSink for closures.
pub struct ClosureEventSink<F>(pub F);

impl<F> EventSink for ClosureEventSink<F>
where
    F: Fn(&str, &str) + Send + Sync,
{
    fn on_event(&self, msg: &str, details_json: &str) {
        (self.0)(msg, details_json)
    }
}
