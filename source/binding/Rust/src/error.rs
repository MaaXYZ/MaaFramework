use std::ffi::NulError;
use std::str::Utf8Error;
use thiserror::Error;

#[derive(Error, Debug)]
pub enum MaaError {
    #[error("Null pointer exception")]
    NullPointer,

    #[error("Invalid arguments: {0}")]
    InvalidArgument(String),

    #[error("Invalid value: {0}")]
    InvalidValue(i64),

    #[error("MaaFramework internal error: status {0}")]
    FrameworkError(i32),

    #[error("String conversion error: {0}")]
    Utf8Error(#[from] Utf8Error),

    #[error("CString creation error: {0}")]
    NulError(#[from] NulError),

    #[error("Timeout")]
    Timeout,

    #[error("Device connection failed")]
    DeviceConnectionFailed,

    #[error("Invalid configuration: {0}")]
    InvalidConfig(String),

    #[error("Resource not loaded")]
    ResourceNotLoaded,

    #[error("Context not initialized")]
    ContextNotInitialized,

    #[error("Task failed")]
    TaskFailed,

    #[error("JSON error: {0}")]
    JsonError(#[from] serde_json::Error),

    #[error("Image conversion error")]
    ImageConversionError,
}

pub type MaaResult<T> = Result<T, MaaError>;

impl From<i32> for MaaError {
    fn from(status: i32) -> Self {
        MaaError::FrameworkError(status)
    }
}
