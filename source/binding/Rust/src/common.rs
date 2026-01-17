//! Common types and data structures used throughout the SDK.

use crate::sys;
use std::fmt;

/// Status of an asynchronous operation.
///
/// Most SDK operations are asynchronous and return immediately with an ID.
/// Use this status to check completion state.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, serde::Serialize, serde::Deserialize)]
pub struct MaaStatus(pub i32);

/// Unique identifier for operations, tasks, and nodes.
pub type MaaId = i64;

impl MaaStatus {
    pub const INVALID: Self = Self(sys::MaaStatusEnum_MaaStatus_Invalid as i32);
    pub const PENDING: Self = Self(sys::MaaStatusEnum_MaaStatus_Pending as i32);
    pub const RUNNING: Self = Self(sys::MaaStatusEnum_MaaStatus_Running as i32);
    pub const SUCCEEDED: Self = Self(sys::MaaStatusEnum_MaaStatus_Succeeded as i32);
    pub const FAILED: Self = Self(sys::MaaStatusEnum_MaaStatus_Failed as i32);

    /// Check if the operation succeeded.
    pub fn is_success(&self) -> bool {
        *self == Self::SUCCEEDED
    }

    /// Check if the operation succeeded (alias for is_success).
    pub fn succeeded(&self) -> bool {
        *self == Self::SUCCEEDED
    }

    /// Check if the operation failed.
    pub fn is_failed(&self) -> bool {
        *self == Self::FAILED
    }

    /// Check if the operation failed (alias for is_failed).
    pub fn failed(&self) -> bool {
        *self == Self::FAILED
    }

    /// Check if the operation is done (succeeded or failed).
    pub fn done(&self) -> bool {
        *self == Self::SUCCEEDED || *self == Self::FAILED
    }

    /// Check if the operation is pending.
    pub fn pending(&self) -> bool {
        *self == Self::PENDING
    }

    /// Check if the operation is running.
    pub fn running(&self) -> bool {
        *self == Self::RUNNING
    }
}

impl fmt::Display for MaaStatus {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            Self::INVALID => write!(f, "Invalid"),
            Self::PENDING => write!(f, "Pending"),
            Self::RUNNING => write!(f, "Running"),
            Self::SUCCEEDED => write!(f, "Succeeded"),
            Self::FAILED => write!(f, "Failed"),
            _ => write!(f, "Unknown({})", self.0),
        }
    }
}

pub fn check_bool(ret: sys::MaaBool) -> crate::MaaResult<()> {
    if ret != 0 {
        Ok(())
    } else {
        Err(crate::MaaError::FrameworkError(0))
    }
}

impl From<i32> for MaaStatus {
    fn from(value: i32) -> Self {
        Self(value)
    }
}

/// A rectangle representing a region on screen.
#[derive(Debug, Clone, Copy, PartialEq, Eq, serde::Serialize, serde::Deserialize)]
pub struct Rect {
    pub x: i32,
    pub y: i32,
    pub width: i32,
    pub height: i32,
}

impl From<sys::MaaRect> for Rect {
    fn from(r: sys::MaaRect) -> Self {
        Self {
            x: r.x,
            y: r.y,
            width: r.width,
            height: r.height,
        }
    }
}

/// A point representing a location on screen.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default, serde::Serialize, serde::Deserialize)]
pub struct Point {
    pub x: i32,
    pub y: i32,
}

impl Point {
    pub fn new(x: i32, y: i32) -> Self {
        Self { x, y }
    }
}

// ============================================================================
// Gamepad Types (Windows only, requires ViGEm Bus Driver)
// ============================================================================

/// Virtual gamepad type for GamepadController.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(u64)]
pub enum GamepadType {
    /// Microsoft Xbox 360 Controller (wired)
    Xbox360 = 0,
    /// Sony DualShock 4 Controller (wired)
    DualShock4 = 1,
}

/// Gamepad contact (analog stick or trigger) for touch mapping.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(i32)]
pub enum GamepadContact {
    /// Left analog stick: x/y range -32768~32767
    LeftStick = 0,
    /// Right analog stick: x/y range -32768~32767
    RightStick = 1,
    /// Left trigger: pressure 0~255
    LeftTrigger = 2,
    /// Right trigger: pressure 0~255
    RightTrigger = 3,
}

bitflags::bitflags! {
    /// Gamepad button flags (XUSB protocol values).
    ///
    /// Use bitwise OR to combine multiple buttons.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    pub struct GamepadButton: u32 {
        // D-pad
        const DPAD_UP = 0x0001;
        const DPAD_DOWN = 0x0002;
        const DPAD_LEFT = 0x0004;
        const DPAD_RIGHT = 0x0008;

        // Control buttons
        const START = 0x0010;
        const BACK = 0x0020;
        const LEFT_THUMB = 0x0040;  // L3
        const RIGHT_THUMB = 0x0080; // R3

        // Shoulder buttons
        const LB = 0x0100; // Left Bumper / L1
        const RB = 0x0200; // Right Bumper / R1

        // Guide button
        const GUIDE = 0x0400;

        // Face buttons (Xbox layout)
        const A = 0x1000;
        const B = 0x2000;
        const X = 0x4000;
        const Y = 0x8000;

        // DS4 special buttons
        const PS = 0x10000;
        const TOUCHPAD = 0x20000;
    }
}

impl GamepadButton {
    // DS4 face button aliases
    pub const CROSS: Self = Self::A;
    pub const CIRCLE: Self = Self::B;
    pub const SQUARE: Self = Self::X;
    pub const TRIANGLE: Self = Self::Y;
    pub const L1: Self = Self::LB;
    pub const R1: Self = Self::RB;
    pub const L3: Self = Self::LEFT_THUMB;
    pub const R3: Self = Self::RIGHT_THUMB;
    pub const OPTIONS: Self = Self::START;
    pub const SHARE: Self = Self::BACK;
}

// ============================================================================
// Controller Feature Flags
// ============================================================================

bitflags::bitflags! {
    /// Controller feature flags for CustomController.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
    pub struct ControllerFeature: u64 {
        const USE_MOUSE_DOWN_UP_INSTEAD_OF_CLICK = 1;
        const USE_KEY_DOWN_UP_INSTEAD_OF_CLICK = 1 << 1;
    }
}

// ============================================================================
// ADB Controller Methods
// ============================================================================

bitflags::bitflags! {
    /// ADB screencap method flags.
    ///
    /// Use bitwise OR to set the methods you need.
    /// MaaFramework will test all provided methods and use the fastest available one.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    pub struct AdbScreencapMethod: u64 {
        const ENCODE_TO_FILE_AND_PULL = 1;
        const ENCODE = 1 << 1;
        const RAW_WITH_GZIP = 1 << 2;
        const RAW_BY_NETCAT = 1 << 3;
        const MINICAP_DIRECT = 1 << 4;
        const MINICAP_STREAM = 1 << 5;
        const EMULATOR_EXTRAS = 1 << 6;
        const ALL = !0;
    }
}

impl AdbScreencapMethod {
    /// Default methods (all except RawByNetcat, MinicapDirect, MinicapStream)
    pub const DEFAULT: Self = Self::from_bits_truncate(
        Self::ALL.bits()
            & !Self::RAW_BY_NETCAT.bits()
            & !Self::MINICAP_DIRECT.bits()
            & !Self::MINICAP_STREAM.bits(),
    );
}

impl Default for AdbScreencapMethod {
    fn default() -> Self {
        Self::DEFAULT
    }
}

bitflags::bitflags! {
    /// ADB input method flags.
    ///
    /// Use bitwise OR to set the methods you need.
    /// MaaFramework will select the first available method according to priority.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    pub struct AdbInputMethod: u64 {
        const ADB_SHELL = 1;
        const MINITOUCH_AND_ADB_KEY = 1 << 1;
        const MAATOUCH = 1 << 2;
        const EMULATOR_EXTRAS = 1 << 3;
        const ALL = !0;
    }
}

impl AdbInputMethod {
    /// Default methods (all except EmulatorExtras)
    pub const DEFAULT: Self =
        Self::from_bits_truncate(Self::ALL.bits() & !Self::EMULATOR_EXTRAS.bits());
}

impl Default for AdbInputMethod {
    fn default() -> Self {
        Self::DEFAULT
    }
}

// ============================================================================
// Win32 Controller Methods
// ============================================================================

bitflags::bitflags! {
    /// Win32 screencap method (select ONE only, no bitwise OR).
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    pub struct Win32ScreencapMethod: u64 {
        const GDI = 1;
        const FRAME_POOL = 1 << 1;
        const DXGI_DESKTOP_DUP = 1 << 2;
        const DXGI_DESKTOP_DUP_WINDOW = 1 << 3;
        const PRINT_WINDOW = 1 << 4;
        const SCREEN_DC = 1 << 5;
    }
}

bitflags::bitflags! {
    /// Win32 input method (select ONE only, no bitwise OR).
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    pub struct Win32InputMethod: u64 {
        const SEIZE = 1;
        const SEND_MESSAGE = 1 << 1;
        const POST_MESSAGE = 1 << 2;
        const LEGACY_EVENT = 1 << 3;
        const POST_THREAD_MESSAGE = 1 << 4;
        const SEND_MESSAGE_WITH_CURSOR_POS = 1 << 5;
        const POST_MESSAGE_WITH_CURSOR_POS = 1 << 6;
    }
}

/// Details of a recognition operation result.
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct RecognitionDetail {
    /// Name of the node that performed recognition
    pub node_name: String,
    /// Algorithm used (e.g., "TemplateMatch", "OCR")
    pub algorithm: String,
    /// Whether recognition was successful
    pub hit: bool,
    /// Bounding box of the recognized region
    pub box_rect: Rect,
    /// Algorithm-specific detail JSON
    pub detail: serde_json::Value,
    /// Raw screenshot (PNG encoded, only valid in debug mode)
    #[serde(skip)]
    pub raw_image: Option<Vec<u8>>,
    /// Debug draw images (PNG encoded, only valid in debug mode)
    #[serde(skip)]
    pub draw_images: Vec<Vec<u8>>,
}

impl RecognitionDetail {
    pub fn as_template_match_result(&self) -> Option<TemplateMatchResult> {
        serde_json::from_value(self.detail.clone()).ok()
    }

    pub fn as_feature_match_result(&self) -> Option<FeatureMatchResult> {
        serde_json::from_value(self.detail.clone()).ok()
    }

    pub fn as_color_match_result(&self) -> Option<ColorMatchResult> {
        serde_json::from_value(self.detail.clone()).ok()
    }

    pub fn as_ocr_result(&self) -> Option<OCRResult> {
        serde_json::from_value(self.detail.clone()).ok()
    }

    pub fn as_neural_network_result(&self) -> Option<NeuralNetworkResult> {
        serde_json::from_value(self.detail.clone()).ok()
    }

    pub fn as_custom_result(&self) -> Option<CustomRecognitionResult> {
        serde_json::from_value(self.detail.clone()).ok()
    }
}

/// Details of an action operation result.
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct ActionDetail {
    /// Name of the node that performed the action
    pub node_name: String,
    /// Action type (e.g., "Click", "Swipe")
    pub action: String,
    /// Target bounding box
    pub box_rect: Rect,
    /// Whether action was successful
    pub success: bool,
    /// Action-specific detail JSON
    pub detail: serde_json::Value,
}

impl ActionDetail {
    pub fn as_click_result(&self) -> Option<ClickActionResult> {
        serde_json::from_value(self.detail.clone()).ok()
    }

    pub fn as_long_press_result(&self) -> Option<LongPressActionResult> {
        serde_json::from_value(self.detail.clone()).ok()
    }

    pub fn as_swipe_result(&self) -> Option<SwipeActionResult> {
        serde_json::from_value(self.detail.clone()).ok()
    }

    pub fn as_multi_swipe_result(&self) -> Option<MultiSwipeActionResult> {
        serde_json::from_value(self.detail.clone()).ok()
    }

    pub fn as_click_key_result(&self) -> Option<ClickKeyActionResult> {
        serde_json::from_value(self.detail.clone()).ok()
    }

    pub fn as_input_text_result(&self) -> Option<InputTextActionResult> {
        serde_json::from_value(self.detail.clone()).ok()
    }

    pub fn as_app_result(&self) -> Option<AppActionResult> {
        serde_json::from_value(self.detail.clone()).ok()
    }

    pub fn as_scroll_result(&self) -> Option<ScrollActionResult> {
        serde_json::from_value(self.detail.clone()).ok()
    }

    pub fn as_touch_result(&self) -> Option<TouchActionResult> {
        serde_json::from_value(self.detail.clone()).ok()
    }

    pub fn as_shell_result(&self) -> Option<ShellActionResult> {
        serde_json::from_value(self.detail.clone()).ok()
    }
}

// ============================================================================
// Action Result Types
// ============================================================================

/// Result of a Click action.
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct ClickActionResult {
    pub point: Point,
    pub contact: i32,
}

/// Result of a LongPress action.
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct LongPressActionResult {
    pub point: Point,
    pub duration: i32,
    pub contact: i32,
}

/// Result of a Swipe action.
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct SwipeActionResult {
    pub begin: Point,
    pub end: Vec<Point>,
    #[serde(default)]
    pub end_hold: Vec<i32>,
    #[serde(default)]
    pub duration: Vec<i32>,
    #[serde(default)]
    pub only_hover: bool,
    #[serde(default)]
    pub starting: i32,
    pub contact: i32,
}

/// Result of a MultiSwipe action.
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct MultiSwipeActionResult {
    pub swipes: Vec<SwipeActionResult>,
}

/// Result of a ClickKey action.
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct ClickKeyActionResult {
    pub keycode: Vec<i32>,
}

/// Result of a LongPressKey action.
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct LongPressKeyActionResult {
    pub keycode: Vec<i32>,
    pub duration: i32,
}

/// Result of an InputText action.
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct InputTextActionResult {
    pub text: String,
}

/// Result of a StartApp or StopApp action.
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct AppActionResult {
    pub package: String,
}

/// Result of a Scroll action.
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct ScrollActionResult {
    pub dx: i32,
    pub dy: i32,
}

/// Result of a TouchDown, TouchMove, or TouchUp action.
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct TouchActionResult {
    pub contact: i32,
    pub point: Point,
    #[serde(default)]
    pub pressure: i32,
}

/// Result of a Shell action.
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct ShellActionResult {
    pub cmd: String,
    pub timeout: i32,
    pub success: bool,
    pub output: String,
}

/// Details of a pipeline node execution.
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct NodeDetail {
    pub node_name: String,
    /// ID of the recognition operation
    pub reco_id: MaaId,
    /// ID of the action operation
    pub act_id: MaaId,
    /// Whether the node completed execution
    pub completed: bool,
}

/// Details of a task execution.
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct TaskDetail {
    /// Entry point node name
    pub entry: String,
    /// List of node IDs that were executed
    pub node_id_list: Vec<MaaId>,
    /// Final status of the task
    pub status: MaaStatus,
}

/// Recognition algorithm types.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, serde::Serialize, serde::Deserialize)]
#[serde(rename_all = "PascalCase")]
pub enum AlgorithmEnum {
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

impl AlgorithmEnum {
    pub fn as_str(&self) -> &'static str {
        match self {
            Self::DirectHit => "DirectHit",
            Self::TemplateMatch => "TemplateMatch",
            Self::FeatureMatch => "FeatureMatch",
            Self::ColorMatch => "ColorMatch",
            Self::OCR => "OCR",
            Self::NeuralNetworkClassify => "NeuralNetworkClassify",
            Self::NeuralNetworkDetect => "NeuralNetworkDetect",
            Self::And => "And",
            Self::Or => "Or",
            Self::Custom => "Custom",
        }
    }
}

/// Action types.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, serde::Serialize, serde::Deserialize)]
#[serde(rename_all = "PascalCase")]
pub enum ActionEnum {
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

impl ActionEnum {
    pub fn as_str(&self) -> &'static str {
        match self {
            Self::DoNothing => "DoNothing",
            Self::Click => "Click",
            Self::LongPress => "LongPress",
            Self::Swipe => "Swipe",
            Self::MultiSwipe => "MultiSwipe",
            Self::TouchDown => "TouchDown",
            Self::TouchMove => "TouchMove",
            Self::TouchUp => "TouchUp",
            Self::ClickKey => "ClickKey",
            Self::LongPressKey => "LongPressKey",
            Self::KeyDown => "KeyDown",
            Self::KeyUp => "KeyUp",
            Self::InputText => "InputText",
            Self::StartApp => "StartApp",
            Self::StopApp => "StopApp",
            Self::StopTask => "StopTask",
            Self::Scroll => "Scroll",
            Self::Command => "Command",
            Self::Shell => "Shell",
            Self::Custom => "Custom",
        }
    }
}

/// Notification type for event callbacks.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum NotificationType {
    Starting,
    Succeeded,
    Failed,
    Unknown,
}

impl NotificationType {
    pub fn from_message(msg: &str) -> Self {
        if msg.ends_with(".Starting") {
            Self::Starting
        } else if msg.ends_with(".Succeeded") {
            Self::Succeeded
        } else if msg.ends_with(".Failed") {
            Self::Failed
        } else {
            Self::Unknown
        }
    }
}

// --- Result types ---

#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct BoxAndScore {
    #[serde(rename = "box")]
    pub box_rect: (i32, i32, i32, i32),
    pub score: f64,
}

#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct BoxAndCount {
    #[serde(rename = "box")]
    pub box_rect: (i32, i32, i32, i32),
    pub count: i32,
}

pub type TemplateMatchResult = BoxAndScore;
pub type FeatureMatchResult = BoxAndCount;
pub type ColorMatchResult = BoxAndCount;

#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct OCRResult {
    #[serde(flatten)]
    pub base: BoxAndScore,
    pub text: String,
}

#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct NeuralNetworkResult {
    #[serde(flatten)]
    pub base: BoxAndScore,
    pub cls_index: i32,
    pub label: String,
}

pub type NeuralNetworkClassifyResult = NeuralNetworkResult;
pub type NeuralNetworkDetectResult = NeuralNetworkResult;

#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct CustomRecognitionResult {
    #[serde(rename = "box")]
    pub box_rect: (i32, i32, i32, i32),
    pub detail: serde_json::Value,
}
