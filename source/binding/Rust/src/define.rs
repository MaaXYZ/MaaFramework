use std::os::raw::{c_char, c_int, c_void};

pub type MaaBool = u8;
pub type MaaSize = u64;
pub const MAA_NULL_SIZE: MaaSize = u64::MAX;

pub type MaaId = i64;
pub type MaaCtrlId = MaaId;
pub type MaaResId = MaaId;
pub type MaaTaskId = MaaId;
pub type MaaRecoId = MaaId;
pub type MaaActId = MaaId;
pub type MaaNodeId = MaaId;
pub type MaaSinkId = MaaId;
pub const MAA_INVALID_ID: MaaId = 0;

pub type MaaStringBufferHandle = *mut c_void;
pub type MaaImageBufferHandle = *mut c_void;
pub type MaaStringListBufferHandle = *mut c_void;
pub type MaaImageListBufferHandle = *mut c_void;
pub type MaaRectHandle = *mut c_void;

pub type MaaResourceHandle = *mut c_void;
pub type MaaControllerHandle = *mut c_void;
pub type MaaTaskerHandle = *mut c_void;
pub type MaaContextHandle = *mut c_void;

pub type MaaStatus = i32;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum Status {
    Invalid = 0,
    Pending = 1000,
    Running = 2000,
    Succeeded = 3000,
    Failed = 4000,
}

impl From<MaaStatus> for Status {
    fn from(value: MaaStatus) -> Self {
        match value {
            0 => Status::Invalid,
            1000 => Status::Pending,
            2000 => Status::Running,
            3000 => Status::Succeeded,
            4000 => Status::Failed,
            _ => Status::Invalid,
        }
    }
}

impl Status {
    pub fn done(&self) -> bool {
        matches!(self, Status::Succeeded | Status::Failed)
    }

    pub fn succeeded(&self) -> bool {
        matches!(self, Status::Succeeded)
    }

    pub fn failed(&self) -> bool {
        matches!(self, Status::Failed)
    }

    pub fn pending(&self) -> bool {
        matches!(self, Status::Pending)
    }

    pub fn running(&self) -> bool {
        matches!(self, Status::Running)
    }
}

pub type MaaLoggingLevel = i32;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum LoggingLevel {
    Off = 0,
    Fatal = 1,
    Error = 2,
    Warn = 3,
    Info = 4,
    Debug = 5,
    Trace = 6,
    All = 7,
}

pub type MaaOption = i32;
pub type MaaOptionValue = *mut c_void;
pub type MaaOptionValueSize = u64;

pub type MaaGlobalOption = MaaOption;
pub type MaaCtrlOption = MaaOption;
pub type MaaResOption = MaaOption;
pub type MaaTaskerOption = MaaOption;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum GlobalOption {
    Invalid = 0,
    LogDir = 1,
    SaveDraw = 2,
    StdoutLevel = 4,
    DebugMode = 6,
    SaveOnError = 7,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum CtrlOption {
    Invalid = 0,
    ScreenshotTargetLongSide = 1,
    ScreenshotTargetShortSide = 2,
    ScreenshotUseRawSize = 3,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum InferenceDevice {
    Cpu = -2,
    Auto = -1,
    Gpu0 = 0,
    Gpu1 = 1,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum InferenceExecutionProvider {
    Auto = 0,
    Cpu = 1,
    DirectMl = 2,
    CoreMl = 3,
    Cuda = 4,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum ResOption {
    Invalid = 0,
    InferenceDevice = 1,
    InferenceExecutionProvider = 2,
}

pub type MaaAdbScreencapMethod = u64;

pub mod adb_screencap_method {
    use super::MaaAdbScreencapMethod;

    pub const NONE: MaaAdbScreencapMethod = 0;
    pub const ENCODE_TO_FILE_AND_PULL: MaaAdbScreencapMethod = 1;
    pub const ENCODE: MaaAdbScreencapMethod = 1 << 1;
    pub const RAW_WITH_GZIP: MaaAdbScreencapMethod = 1 << 2;
    pub const RAW_BY_NETCAT: MaaAdbScreencapMethod = 1 << 3;
    pub const MINICAP_DIRECT: MaaAdbScreencapMethod = 1 << 4;
    pub const MINICAP_STREAM: MaaAdbScreencapMethod = 1 << 5;
    pub const EMULATOR_EXTRAS: MaaAdbScreencapMethod = 1 << 6;
    pub const ALL: MaaAdbScreencapMethod = !NONE;
    pub const DEFAULT: MaaAdbScreencapMethod =
        ALL & (!RAW_BY_NETCAT) & (!MINICAP_DIRECT) & (!MINICAP_STREAM);
}

pub type MaaAdbInputMethod = u64;

pub mod adb_input_method {
    use super::MaaAdbInputMethod;

    pub const NONE: MaaAdbInputMethod = 0;
    pub const ADB_SHELL: MaaAdbInputMethod = 1;
    pub const MINITOUCH_AND_ADB_KEY: MaaAdbInputMethod = 1 << 1;
    pub const MAATOUCH: MaaAdbInputMethod = 1 << 2;
    pub const EMULATOR_EXTRAS: MaaAdbInputMethod = 1 << 3;
    pub const ALL: MaaAdbInputMethod = !NONE;
    pub const DEFAULT: MaaAdbInputMethod = ALL & (!EMULATOR_EXTRAS);
}

pub type MaaWin32ScreencapMethod = u64;

pub mod win32_screencap_method {
    use super::MaaWin32ScreencapMethod;

    pub const NONE: MaaWin32ScreencapMethod = 0;
    pub const GDI: MaaWin32ScreencapMethod = 1;
    pub const FRAME_POOL: MaaWin32ScreencapMethod = 1 << 1;
    pub const DXGI_DESKTOP_DUP: MaaWin32ScreencapMethod = 1 << 2;
    pub const DXGI_DESKTOP_DUP_WINDOW: MaaWin32ScreencapMethod = 1 << 3;
    pub const PRINT_WINDOW: MaaWin32ScreencapMethod = 1 << 4;
    pub const SCREEN_DC: MaaWin32ScreencapMethod = 1 << 5;
}

pub type MaaWin32InputMethod = u64;

pub mod win32_input_method {
    use super::MaaWin32InputMethod;

    pub const NONE: MaaWin32InputMethod = 0;
    pub const SEIZE: MaaWin32InputMethod = 1;
    pub const SEND_MESSAGE: MaaWin32InputMethod = 1 << 1;
    pub const POST_MESSAGE: MaaWin32InputMethod = 1 << 2;
    pub const LEGACY_EVENT: MaaWin32InputMethod = 1 << 3;
    pub const POST_THREAD_MESSAGE: MaaWin32InputMethod = 1 << 4;
}

pub type MaaDbgControllerType = u64;

pub mod dbg_controller_type {
    use super::MaaDbgControllerType;

    pub const NONE: MaaDbgControllerType = 0;
    pub const CAROUSEL_IMAGE: MaaDbgControllerType = 1;
    pub const REPLAY_RECORDING: MaaDbgControllerType = 1 << 1;
}

pub type MaaControllerFeature = u64;

pub mod controller_feature {
    use super::MaaControllerFeature;

    pub const NONE: MaaControllerFeature = 0;
    pub const USE_MOUSE_DOWN_AND_UP_INSTEAD_OF_CLICK: MaaControllerFeature = 1;
    pub const USE_KEYBOARD_DOWN_AND_UP_INSTEAD_OF_CLICK: MaaControllerFeature = 1 << 1;
}

#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct Rect {
    pub x: i32,
    pub y: i32,
    pub width: i32,
    pub height: i32,
}

impl Rect {
    pub fn new(x: i32, y: i32, width: i32, height: i32) -> Self {
        Self { x, y, width, height }
    }
}

pub type MaaEventCallback = extern "C" fn(
    handle: *mut c_void,
    message: *const c_char,
    details_json: *const c_char,
    trans_arg: *mut c_void,
);

pub type MaaCustomRecognitionCallback = extern "C" fn(
    context: MaaContextHandle,
    task_id: MaaTaskId,
    node_name: *const c_char,
    custom_recognition_name: *const c_char,
    custom_recognition_param: *const c_char,
    image: MaaImageBufferHandle,
    roi: *const Rect,
    trans_arg: *mut c_void,
    out_box: *mut Rect,
    out_detail: MaaStringBufferHandle,
) -> MaaBool;

pub type MaaCustomActionCallback = extern "C" fn(
    context: MaaContextHandle,
    task_id: MaaTaskId,
    node_name: *const c_char,
    custom_action_name: *const c_char,
    custom_action_param: *const c_char,
    reco_id: MaaRecoId,
    box_: *const Rect,
    trans_arg: *mut c_void,
) -> MaaBool;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Algorithm {
    DirectHit,
    TemplateMatch,
    FeatureMatch,
    ColorMatch,
    Ocr,
    NeuralNetworkClassify,
    NeuralNetworkDetect,
    Custom,
}

impl From<&str> for Algorithm {
    fn from(s: &str) -> Self {
        match s {
            "DirectHit" => Algorithm::DirectHit,
            "TemplateMatch" => Algorithm::TemplateMatch,
            "FeatureMatch" => Algorithm::FeatureMatch,
            "ColorMatch" => Algorithm::ColorMatch,
            "OCR" => Algorithm::Ocr,
            "NeuralNetworkClassify" => Algorithm::NeuralNetworkClassify,
            "NeuralNetworkDetect" => Algorithm::NeuralNetworkDetect,
            "Custom" => Algorithm::Custom,
            _ => Algorithm::DirectHit,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Action {
    DoNothing,
    Click,
    LongPress,
    Swipe,
    MultiSwipe,
    ClickKey,
    LongPressKey,
    InputText,
    StartApp,
    StopApp,
    Command,
    Custom,
    StopTask,
}

impl From<&str> for Action {
    fn from(s: &str) -> Self {
        match s {
            "DoNothing" => Action::DoNothing,
            "Click" => Action::Click,
            "LongPress" => Action::LongPress,
            "Swipe" => Action::Swipe,
            "MultiSwipe" => Action::MultiSwipe,
            "ClickKey" => Action::ClickKey,
            "LongPressKey" => Action::LongPressKey,
            "InputText" => Action::InputText,
            "StartApp" => Action::StartApp,
            "StopApp" => Action::StopApp,
            "Command" => Action::Command,
            "Custom" => Action::Custom,
            "StopTask" => Action::StopTask,
            _ => Action::DoNothing,
        }
    }
}

