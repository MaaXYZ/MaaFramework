use serde::de::DeserializeOwned;
use serde::{Deserialize, Deserializer, Serialize};
use serde_json::Value;

// --- Custom Deserializers for Scalar/Array Polymorphism ---
// The C API may return either a scalar or an array for some fields.

/// Deserialize a value that can be either T or Vec<T> into Vec<T>
fn scalar_or_vec<'de, D, T>(deserializer: D) -> Result<Option<Vec<T>>, D::Error>
where
    D: Deserializer<'de>,
    T: DeserializeOwned,
{
    let value: Option<Value> = Option::deserialize(deserializer)?;
    match value {
        None => Ok(None),
        Some(Value::Array(arr)) => {
            let vec: Vec<T> = arr
                .into_iter()
                .map(|v| serde_json::from_value(v))
                .collect::<Result<Vec<T>, _>>()
                .map_err(serde::de::Error::custom)?;
            Ok(Some(vec))
        }
        Some(v) => {
            let single: T = serde_json::from_value(v).map_err(serde::de::Error::custom)?;
            Ok(Some(vec![single]))
        }
    }
}

// --- Common Types ---

pub type Rect = (i32, i32, i32, i32); // x, y, width, height
pub type Roi = (i32, i32, i32, i32);

/// Target can be:
/// - true: recognized position
/// - "NodeName": position from previously executed node
/// - [x, y]: point (2 elements)
/// - [x, y, w, h]: area (4 elements)
#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(untagged)]
pub enum Target {
    Bool(bool),
    Name(String),
    Point((i32, i32)),
    Rect(Rect),
}

impl Default for Target {
    fn default() -> Self {
        Target::Bool(true)
    }
}

// --- Node Attribute ---

#[derive(Serialize, Deserialize, Debug, Clone, Default)]
pub struct NodeAttr {
    #[serde(default)]
    pub name: String,
    #[serde(default)]
    pub jump_back: bool,
    #[serde(default)]
    pub anchor: bool,
}

// --- Wait Freezes ---

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct WaitFreezes {
    #[serde(default = "default_wait_time")]
    pub time: i32,
    #[serde(default)]
    pub target: Target,
    #[serde(default)]
    pub target_offset: Rect,
    #[serde(default = "default_wait_threshold")]
    pub threshold: f64,
    #[serde(default = "default_wait_method")]
    pub method: i32,
    #[serde(default = "default_rate_limit")]
    pub rate_limit: i32,
    #[serde(default = "default_timeout")]
    pub timeout: i32,
}

impl Default for WaitFreezes {
    fn default() -> Self {
        Self {
            time: default_wait_time(),
            target: Target::default(),
            target_offset: (0, 0, 0, 0),
            threshold: default_wait_threshold(),
            method: default_wait_method(),
            rate_limit: default_rate_limit(),
            timeout: default_timeout(),
        }
    }
}

// --- Recognition ---

/// Recognition type enum matching C API
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum RecognitionType {
    DirectHit,
    TemplateMatch,
    FeatureMatch,
    ColorMatch,
    OCR,
    NeuralNetworkClassify,
    NeuralNetworkDetect,
    And,
    Or,
    Custom,
}

/// Recognition struct with type and param fields (matching C API JSON format)
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Recognition {
    #[serde(rename = "type")]
    pub recognition_type: RecognitionType,
    #[serde(default)]
    pub param: RecognitionParam,
}

/// Recognition parameter - holds all possible recognition parameters
#[derive(Serialize, Deserialize, Debug, Clone, Default)]
pub struct RecognitionParam {
    // DirectHit / common fields
    #[serde(default)]
    pub roi: Option<Target>,
    #[serde(default)]
    pub roi_offset: Option<Rect>,

    // TemplateMatch
    #[serde(default, deserialize_with = "scalar_or_vec")]
    pub template: Option<Vec<String>>,
    #[serde(default, deserialize_with = "scalar_or_vec")]
    pub threshold: Option<Vec<f64>>,
    #[serde(default)]
    pub order_by: Option<String>,
    #[serde(default)]
    pub index: Option<i32>,
    #[serde(default)]
    pub method: Option<i32>,
    #[serde(default)]
    pub green_mask: Option<bool>,

    // FeatureMatch
    #[serde(default)]
    pub detector: Option<String>,
    #[serde(default)]
    pub count: Option<i32>,
    #[serde(default)]
    pub ratio: Option<f64>,

    // ColorMatch
    #[serde(default)]
    pub lower: Option<Vec<Vec<i32>>>,
    #[serde(default)]
    pub upper: Option<Vec<Vec<i32>>>,
    #[serde(default)]
    pub connected: Option<bool>,

    // OCR / NeuralNetwork (expected can be Vec<String> for OCR or Vec<i32> for NN)
    #[serde(default)]
    pub expected: Option<Vec<Value>>,
    #[serde(default)]
    pub replace: Option<Vec<Vec<String>>>,
    #[serde(default)]
    pub only_rec: Option<bool>,
    #[serde(default)]
    pub model: Option<String>,

    // NeuralNetwork
    #[serde(default)]
    pub labels: Option<Vec<String>>,

    // And/Or
    #[serde(default)]
    pub all_of: Option<Vec<serde_json::Value>>,
    #[serde(default)]
    pub any_of: Option<Vec<serde_json::Value>>,
    #[serde(default)]
    pub box_index: Option<i32>,

    // Custom
    #[serde(default)]
    pub custom_recognition: Option<String>,
    #[serde(default)]
    pub custom_recognition_param: Option<serde_json::Value>,
}

#[derive(Serialize, Deserialize, Debug, Clone, Default)]
pub struct DirectHit {
    #[serde(default)]
    pub roi: Target,
    #[serde(default)]
    pub roi_offset: Rect,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct TemplateMatch {
    pub template: Vec<String>,
    #[serde(default)]
    pub roi: Target,
    #[serde(default)]
    pub roi_offset: Rect,
    #[serde(default = "default_threshold")]
    pub threshold: Vec<f64>,
    #[serde(default = "default_order_by")]
    pub order_by: String,
    #[serde(default)]
    pub index: i32,
    #[serde(default = "default_template_method")]
    pub method: i32,
    #[serde(default)]
    pub green_mask: bool,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct FeatureMatch {
    pub template: Vec<String>,
    #[serde(default)]
    pub roi: Target,
    #[serde(default)]
    pub roi_offset: Rect,
    #[serde(default = "default_detector")]
    pub detector: String,
    #[serde(default = "default_order_by")]
    pub order_by: String,
    #[serde(default = "default_feature_count")]
    pub count: i32,
    #[serde(default)]
    pub index: i32,
    #[serde(default)]
    pub green_mask: bool,
    #[serde(default = "default_feature_ratio")]
    pub ratio: f64,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ColorMatch {
    pub lower: Vec<Vec<i32>>,
    pub upper: Vec<Vec<i32>>,
    #[serde(default)]
    pub roi: Target,
    #[serde(default)]
    pub roi_offset: Rect,
    #[serde(default = "default_order_by")]
    pub order_by: String,
    #[serde(default = "default_color_method")]
    pub method: i32,
    #[serde(default = "default_count_one")]
    pub count: i32,
    #[serde(default)]
    pub index: i32,
    #[serde(default)]
    pub connected: bool,
}

#[derive(Serialize, Deserialize, Debug, Clone, Default)]
pub struct OCR {
    #[serde(default)]
    pub expected: Vec<String>,
    #[serde(default)]
    pub roi: Target,
    #[serde(default)]
    pub roi_offset: Rect,
    #[serde(default = "default_ocr_threshold")]
    pub threshold: f64,
    #[serde(default)]
    pub replace: Vec<Vec<String>>,
    #[serde(default = "default_order_by")]
    pub order_by: String,
    #[serde(default)]
    pub index: i32,
    #[serde(default)]
    pub only_rec: bool,
    #[serde(default)]
    pub model: String,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct NeuralNetworkClassify {
    pub model: String,
    #[serde(default)]
    pub expected: Vec<i32>,
    #[serde(default)]
    pub roi: Target,
    #[serde(default)]
    pub roi_offset: Rect,
    #[serde(default)]
    pub labels: Vec<String>,
    #[serde(default = "default_order_by")]
    pub order_by: String,
    #[serde(default)]
    pub index: i32,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct NeuralNetworkDetect {
    pub model: String,
    #[serde(default)]
    pub expected: Vec<i32>,
    #[serde(default)]
    pub roi: Target,
    #[serde(default)]
    pub roi_offset: Rect,
    #[serde(default)]
    pub labels: Vec<String>,
    #[serde(default = "default_detect_threshold")]
    pub threshold: Vec<f64>,
    #[serde(default = "default_order_by")]
    pub order_by: String,
    #[serde(default)]
    pub index: i32,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct CustomRecognition {
    pub custom_recognition: String,
    #[serde(default)]
    pub roi: Target,
    #[serde(default)]
    pub roi_offset: Rect,
    #[serde(default)]
    pub custom_recognition_param: Value,
}

#[derive(Serialize, Deserialize, Debug, Clone, Default)]
pub struct And {
    #[serde(default)]
    pub all_of: Vec<Value>, // Using Value because recursive types are tricky in simple structs, or we could box Recognition
    #[serde(default)]
    pub box_index: i32,
}

#[derive(Serialize, Deserialize, Debug, Clone, Default)]
pub struct Or {
    #[serde(default)]
    pub any_of: Vec<Value>,
}

// --- Action ---

/// Action type enum matching C API
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum ActionType {
    DoNothing,
    Click,
    LongPress,
    Swipe,
    MultiSwipe,
    TouchDown,
    TouchMove,
    TouchUp,
    ClickKey,
    LongPressKey,
    KeyDown,
    KeyUp,
    InputText,
    StartApp,
    StopApp,
    StopTask,
    Scroll,
    Command,
    Shell,
    Custom,
}

/// Action struct with type and param fields (matching C API JSON format)
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Action {
    #[serde(rename = "type")]
    pub action_type: ActionType,
    #[serde(default)]
    pub param: ActionParam,
}

/// Action parameter - holds all possible action parameters
#[derive(Serialize, Deserialize, Debug, Clone, Default)]
pub struct ActionParam {
    // Click / Touch common
    #[serde(default)]
    pub target: Option<Target>,
    #[serde(default)]
    pub target_offset: Option<Rect>,
    #[serde(default)]
    pub contact: Option<i32>,
    #[serde(default)]
    pub pressure: Option<i32>,

    // LongPress / Swipe duration - C API returns as array e.g. [500]
    #[serde(default, deserialize_with = "scalar_or_vec")]
    pub duration: Option<Vec<i32>>,

    // Swipe
    #[serde(default)]
    pub starting: Option<i32>,
    #[serde(default)]
    pub begin: Option<Target>,
    #[serde(default)]
    pub begin_offset: Option<Rect>,
    #[serde(default)]
    pub end: Option<Vec<Target>>,
    #[serde(default)]
    pub end_offset: Option<Vec<Rect>>,
    #[serde(default)]
    pub end_hold: Option<Vec<i32>>,
    #[serde(default)]
    pub only_hover: Option<bool>,

    // MultiSwipe
    #[serde(default)]
    pub swipes: Option<Vec<serde_json::Value>>,

    // Key
    #[serde(default)]
    pub key: Option<serde_json::Value>, // Can be i32 or Vec<i32>

    // InputText
    #[serde(default)]
    pub input_text: Option<String>,

    // App
    #[serde(default)]
    pub package: Option<String>,

    // Scroll
    #[serde(default)]
    pub dx: Option<i32>,
    #[serde(default)]
    pub dy: Option<i32>,

    // Command
    #[serde(default)]
    pub exec: Option<String>,
    #[serde(default)]
    pub args: Option<Vec<String>>,
    #[serde(default)]
    pub detach: Option<bool>,

    // Shell
    #[serde(default)]
    pub cmd: Option<String>,
    #[serde(default)]
    pub timeout: Option<i32>,

    // Custom
    #[serde(default)]
    pub custom_action: Option<String>,
    #[serde(default)]
    pub custom_action_param: Option<serde_json::Value>,
}

#[derive(Serialize, Deserialize, Debug, Clone, Default)]
pub struct Click {
    #[serde(default)]
    pub target: Target,
    #[serde(default)]
    pub target_offset: Rect,
    #[serde(default)]
    pub contact: i32,
    #[serde(default = "default_pressure")]
    pub pressure: i32,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct LongPress {
    #[serde(default)]
    pub target: Target,
    #[serde(default)]
    pub target_offset: Rect,
    #[serde(default = "default_long_press_duration")]
    pub duration: i32,
    #[serde(default)]
    pub contact: i32,
    #[serde(default = "default_pressure")]
    pub pressure: i32,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Swipe {
    #[serde(default)]
    pub starting: i32, // Used in MultiSwipe
    #[serde(default)]
    pub begin: Target,
    #[serde(default)]
    pub begin_offset: Rect,
    #[serde(default = "default_target_list_true")]
    pub end: Vec<Target>,
    #[serde(default = "default_rect_list_zero")]
    pub end_offset: Vec<Rect>,
    #[serde(default = "default_i32_list_zero")]
    pub end_hold: Vec<i32>,
    #[serde(default = "default_duration_list")]
    pub duration: Vec<i32>,
    #[serde(default)]
    pub only_hover: bool,
    #[serde(default)]
    pub contact: i32,
    #[serde(default = "default_pressure")]
    pub pressure: i32,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct MultiSwipe {
    pub swipes: Vec<Swipe>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Touch {
    #[serde(default)]
    pub contact: i32,
    #[serde(default)]
    pub target: Target,
    #[serde(default)]
    pub target_offset: Rect,
    #[serde(default)]
    pub pressure: i32,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct TouchUp {
    #[serde(default)]
    pub contact: i32,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct KeyList {
    pub key: Vec<i32>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct LongPressKey {
    pub key: Vec<i32>,
    #[serde(default = "default_long_press_duration")]
    pub duration: i32,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Key {
    pub key: i32,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct InputText {
    pub input_text: String,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct App {
    pub package: String,
}

#[derive(Serialize, Deserialize, Debug, Clone, Default)]
pub struct Scroll {
    #[serde(default)]
    pub dx: i32,
    #[serde(default)]
    pub dy: i32,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Command {
    pub exec: String,
    #[serde(default)]
    pub args: Vec<String>,
    #[serde(default)]
    pub detach: bool,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Shell {
    pub cmd: String,
    #[serde(default = "default_timeout")]
    pub timeout: i32,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct CustomAction {
    pub custom_action: String,
    #[serde(default)]
    pub target: Target,
    #[serde(default)]
    pub custom_action_param: Value,
    #[serde(default)]
    pub target_offset: Rect,
}

// --- Pipeline Data ---

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct PipelineData {
    pub recognition: Recognition,
    pub action: Action,
    #[serde(default)]
    pub next: Vec<NodeAttr>,
    #[serde(default = "default_rate_limit")]
    pub rate_limit: i32,
    #[serde(default = "default_timeout")]
    pub timeout: i32,
    #[serde(default)]
    pub on_error: Vec<NodeAttr>,
    #[serde(default)]
    pub anchor: Vec<String>,
    #[serde(default)]
    pub inverse: bool,
    #[serde(default = "default_enabled")]
    pub enabled: bool,
    #[serde(default = "default_pre_delay")]
    pub pre_delay: i32,
    #[serde(default = "default_post_delay")]
    pub post_delay: i32,
    #[serde(default)]
    pub pre_wait_freezes: Option<WaitFreezes>,
    #[serde(default)]
    pub post_wait_freezes: Option<WaitFreezes>,
    #[serde(default = "default_repeat")]
    pub repeat: i32,
    #[serde(default)]
    pub repeat_delay: i32,
    #[serde(default)]
    pub repeat_wait_freezes: Option<WaitFreezes>,
    #[serde(default = "default_max_hit")]
    pub max_hit: u32,
    #[serde(default)]
    pub focus: Option<Value>,
    #[serde(default)]
    pub attach: Option<Value>, // Using Value for arbitrary dict
}

// --- Defaults Helper Functions ---

fn default_wait_time() -> i32 {
    1
}
fn default_wait_threshold() -> f64 {
    0.95
}
fn default_wait_method() -> i32 {
    5
}
fn default_rate_limit() -> i32 {
    1000
}
fn default_timeout() -> i32 {
    20000
}
fn default_threshold() -> Vec<f64> {
    vec![0.7]
}
fn default_order_by() -> String {
    "Horizontal".to_string()
}
fn default_template_method() -> i32 {
    5
}
fn default_detector() -> String {
    "SIFT".to_string()
}
fn default_feature_count() -> i32 {
    4
}
fn default_feature_ratio() -> f64 {
    0.6
}
fn default_color_method() -> i32 {
    4
} // RGB
fn default_count_one() -> i32 {
    1
}
fn default_ocr_threshold() -> f64 {
    0.3
}
fn default_detect_threshold() -> Vec<f64> {
    vec![0.3]
}
fn default_pressure() -> i32 {
    1
}
fn default_long_press_duration() -> i32 {
    1000
}
fn default_target_list_true() -> Vec<Target> {
    vec![Target::Bool(true)]
}
fn default_rect_list_zero() -> Vec<Rect> {
    vec![(0, 0, 0, 0)]
}
fn default_i32_list_zero() -> Vec<i32> {
    vec![0]
}
fn default_duration_list() -> Vec<i32> {
    vec![200]
}
fn default_enabled() -> bool {
    true
}
fn default_pre_delay() -> i32 {
    200
}
fn default_post_delay() -> i32 {
    200
}
fn default_repeat() -> i32 {
    1
}
fn default_max_hit() -> u32 {
    u32::MAX
}
