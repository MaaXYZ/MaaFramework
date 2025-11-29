use serde::{Deserialize, Serialize};
use serde_json::Value;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum NotificationType {
    Starting,
    Succeeded,
    Failed,
    Unknown,
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

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ResourceLoadingDetail {
    pub res_id: i64,
    pub hash: String,
    pub path: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ControllerActionDetail {
    pub ctrl_id: i64,
    pub uuid: String,
    pub action: String,
    pub param: Value,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TaskerTaskDetail {
    pub task_id: i64,
    pub entry: String,
    pub uuid: String,
    pub hash: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct NextListItem {
    pub name: String,
    pub jump_back: bool,
    pub anchor: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct NodeNextListDetail {
    pub task_id: i64,
    pub name: String,
    pub list: Vec<NextListItem>,
    pub focus: Value,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct NodeRecognitionDetail {
    pub task_id: i64,
    pub reco_id: i64,
    pub name: String,
    pub focus: Value,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct NodeActionDetail {
    pub task_id: i64,
    pub action_id: i64,
    pub name: String,
    pub focus: Value,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct NodePipelineNodeDetail {
    pub task_id: i64,
    pub node_id: i64,
    pub name: String,
    pub focus: Value,
}

pub trait ResourceEventSink: Send + Sync {
    fn on_resource_loading(
        &self,
        _noti_type: NotificationType,
        _detail: ResourceLoadingDetail,
    ) {
    }

    fn on_unknown_notification(&self, _msg: &str, _details: &Value) {}
}

pub trait ControllerEventSink: Send + Sync {
    fn on_controller_action(
        &self,
        _noti_type: NotificationType,
        _detail: ControllerActionDetail,
    ) {
    }

    fn on_unknown_notification(&self, _msg: &str, _details: &Value) {}
}

pub trait TaskerEventSink: Send + Sync {
    fn on_tasker_task(&self, _noti_type: NotificationType, _detail: TaskerTaskDetail) {}

    fn on_unknown_notification(&self, _msg: &str, _details: &Value) {}
}

pub trait ContextEventSink: Send + Sync {
    fn on_node_next_list(
        &self,
        _noti_type: NotificationType,
        _detail: NodeNextListDetail,
    ) {
    }

    fn on_node_recognition(
        &self,
        _noti_type: NotificationType,
        _detail: NodeRecognitionDetail,
    ) {
    }

    fn on_node_action(&self, _noti_type: NotificationType, _detail: NodeActionDetail) {}

    fn on_node_pipeline_node(
        &self,
        _noti_type: NotificationType,
        _detail: NodePipelineNodeDetail,
    ) {
    }

    fn on_unknown_notification(&self, _msg: &str, _details: &Value) {}
}

pub fn parse_notification_type(msg: &str) -> NotificationType {
    NotificationType::from(msg)
}

pub fn parse_resource_loading(details: &str) -> Option<ResourceLoadingDetail> {
    serde_json::from_str(details).ok()
}

pub fn parse_controller_action(details: &str) -> Option<ControllerActionDetail> {
    serde_json::from_str(details).ok()
}

pub fn parse_tasker_task(details: &str) -> Option<TaskerTaskDetail> {
    serde_json::from_str(details).ok()
}

pub fn parse_node_next_list(details: &str) -> Option<NodeNextListDetail> {
    serde_json::from_str(details).ok()
}

pub fn parse_node_recognition(details: &str) -> Option<NodeRecognitionDetail> {
    serde_json::from_str(details).ok()
}

pub fn parse_node_action(details: &str) -> Option<NodeActionDetail> {
    serde_json::from_str(details).ok()
}

pub fn parse_node_pipeline_node(details: &str) -> Option<NodePipelineNodeDetail> {
    serde_json::from_str(details).ok()
}

pub mod msg {
    pub const RESOURCE_LOADING_STARTING: &str = "Resource.Loading.Starting";
    pub const RESOURCE_LOADING_SUCCEEDED: &str = "Resource.Loading.Succeeded";
    pub const RESOURCE_LOADING_FAILED: &str = "Resource.Loading.Failed";

    pub const CONTROLLER_ACTION_STARTING: &str = "Controller.Action.Starting";
    pub const CONTROLLER_ACTION_SUCCEEDED: &str = "Controller.Action.Succeeded";
    pub const CONTROLLER_ACTION_FAILED: &str = "Controller.Action.Failed";

    pub const TASKER_TASK_STARTING: &str = "Tasker.Task.Starting";
    pub const TASKER_TASK_SUCCEEDED: &str = "Tasker.Task.Succeeded";
    pub const TASKER_TASK_FAILED: &str = "Tasker.Task.Failed";

    pub const NODE_PIPELINE_NODE_STARTING: &str = "Node.PipelineNode.Starting";
    pub const NODE_PIPELINE_NODE_SUCCEEDED: &str = "Node.PipelineNode.Succeeded";
    pub const NODE_PIPELINE_NODE_FAILED: &str = "Node.PipelineNode.Failed";

    pub const NODE_RECOGNITION_NODE_STARTING: &str = "Node.RecognitionNode.Starting";
    pub const NODE_RECOGNITION_NODE_SUCCEEDED: &str = "Node.RecognitionNode.Succeeded";
    pub const NODE_RECOGNITION_NODE_FAILED: &str = "Node.RecognitionNode.Failed";

    pub const NODE_ACTION_NODE_STARTING: &str = "Node.ActionNode.Starting";
    pub const NODE_ACTION_NODE_SUCCEEDED: &str = "Node.ActionNode.Succeeded";
    pub const NODE_ACTION_NODE_FAILED: &str = "Node.ActionNode.Failed";

    pub const NODE_NEXT_LIST_STARTING: &str = "Node.NextList.Starting";
    pub const NODE_NEXT_LIST_SUCCEEDED: &str = "Node.NextList.Succeeded";
    pub const NODE_NEXT_LIST_FAILED: &str = "Node.NextList.Failed";

    pub const NODE_RECOGNITION_STARTING: &str = "Node.Recognition.Starting";
    pub const NODE_RECOGNITION_SUCCEEDED: &str = "Node.Recognition.Succeeded";
    pub const NODE_RECOGNITION_FAILED: &str = "Node.Recognition.Failed";

    pub const NODE_ACTION_STARTING: &str = "Node.Action.Starting";
    pub const NODE_ACTION_SUCCEEDED: &str = "Node.Action.Succeeded";
    pub const NODE_ACTION_FAILED: &str = "Node.Action.Failed";
}

