use crate::buffer::{ImageBuffer, StringBuffer};
use crate::context::Context;
use crate::define::*;

pub struct CustomRecognitionArgs<'a> {
    pub context: &'a Context,
    pub task_id: MaaTaskId,
    pub node_name: &'a str,
    pub custom_recognition_name: &'a str,
    pub custom_recognition_param: &'a str,
    pub image: &'a ImageBuffer,
    pub roi: Rect,
}

pub struct CustomRecognitionResult {
    pub hit: bool,
    pub box_: Rect,
    pub detail: String,
}

pub trait CustomRecognition: Send + Sync {
    fn run(&self, args: CustomRecognitionArgs) -> Option<CustomRecognitionResult>;
}

pub struct CustomActionArgs<'a> {
    pub context: &'a Context,
    pub task_id: MaaTaskId,
    pub node_name: &'a str,
    pub custom_action_name: &'a str,
    pub custom_action_param: &'a str,
    pub reco_id: MaaRecoId,
    pub box_: Rect,
}

pub trait CustomAction: Send + Sync {
    fn run(&self, args: CustomActionArgs) -> bool;
}

pub struct FnRecognition<F>
where
    F: Fn(CustomRecognitionArgs) -> Option<CustomRecognitionResult> + Send + Sync,
{
    func: F,
}

impl<F> FnRecognition<F>
where
    F: Fn(CustomRecognitionArgs) -> Option<CustomRecognitionResult> + Send + Sync,
{
    pub fn new(func: F) -> Self {
        Self { func }
    }
}

impl<F> CustomRecognition for FnRecognition<F>
where
    F: Fn(CustomRecognitionArgs) -> Option<CustomRecognitionResult> + Send + Sync,
{
    fn run(&self, args: CustomRecognitionArgs) -> Option<CustomRecognitionResult> {
        (self.func)(args)
    }
}

pub struct FnAction<F>
where
    F: Fn(CustomActionArgs) -> bool + Send + Sync,
{
    func: F,
}

impl<F> FnAction<F>
where
    F: Fn(CustomActionArgs) -> bool + Send + Sync,
{
    pub fn new(func: F) -> Self {
        Self { func }
    }
}

impl<F> CustomAction for FnAction<F>
where
    F: Fn(CustomActionArgs) -> bool + Send + Sync,
{
    fn run(&self, args: CustomActionArgs) -> bool {
        (self.func)(args)
    }
}

