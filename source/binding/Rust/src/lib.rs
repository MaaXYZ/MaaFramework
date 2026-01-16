mod buffer;
mod context;
mod controller;
mod custom;
mod define;
mod error;
mod ffi;
mod job;
mod notification;
mod resource;
mod tasker;
mod toolkit;

pub use buffer::*;
pub use context::*;
pub use controller::*;
pub use custom::*;
pub use define::*;
pub use error::*;
pub use job::*;
pub use notification::*;
pub use resource::*;
pub use tasker::*;
pub use toolkit::*;

use libloading::Library as DynLib;
use std::path::Path;
use std::sync::OnceLock;

static FRAMEWORK_LIB: OnceLock<DynLib> = OnceLock::new();
static TOOLKIT_LIB: OnceLock<DynLib> = OnceLock::new();

pub fn load_library(path: impl AsRef<Path>) -> Result<()> {
    let path = path.as_ref();
    let lib = unsafe { DynLib::new(path) }.map_err(|e| MaaError::LibraryLoad(e.to_string()))?;
    FRAMEWORK_LIB
        .set(lib)
        .map_err(|_| MaaError::LibraryLoad("Framework library already loaded".to_string()))?;
    Ok(())
}

pub fn load_toolkit(path: impl AsRef<Path>) -> Result<()> {
    let path = path.as_ref();
    let lib = unsafe { DynLib::new(path) }.map_err(|e| MaaError::LibraryLoad(e.to_string()))?;
    TOOLKIT_LIB
        .set(lib)
        .map_err(|_| MaaError::LibraryLoad("Toolkit library already loaded".to_string()))?;
    Ok(())
}

fn framework() -> &'static DynLib {
    FRAMEWORK_LIB
        .get()
        .expect("MaaFramework library not loaded. Call load_library() first.")
}

fn toolkit() -> &'static DynLib {
    TOOLKIT_LIB
        .get()
        .expect("MaaToolkit library not loaded. Call load_toolkit() first.")
}

