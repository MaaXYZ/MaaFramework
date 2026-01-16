use thiserror::Error;

pub type Result<T> = std::result::Result<T, MaaError>;

#[derive(Error, Debug)]
pub enum MaaError {
    #[error("Library load error: {0}")]
    LibraryLoad(String),

    #[error("Failed to create {0}")]
    CreateFailed(&'static str),

    #[error("Operation failed: {0}")]
    OperationFailed(&'static str),

    #[error("Invalid handle")]
    InvalidHandle,

    #[error("Buffer error: {0}")]
    BufferError(String),

    #[error("JSON error: {0}")]
    JsonError(#[from] serde_json::Error),

    #[error("UTF-8 error: {0}")]
    Utf8Error(#[from] std::string::FromUtf8Error),

    #[error("Null pointer")]
    NullPointer,

    #[error("Task failed")]
    TaskFailed,

    #[error("Connection failed")]
    ConnectionFailed,

    #[error("Resource load failed")]
    ResourceLoadFailed,

    #[error("Invalid id: {0}")]
    InvalidId(i64),
}

