//! Shared test utilities and mock controllers.

#![allow(dead_code)]

use std::path::PathBuf;
use std::sync::atomic::{AtomicUsize, Ordering};

use maa_framework::custom_controller::CustomControllerCallback;
use maa_framework::toolkit::Toolkit;
use maa_framework::{self, sys, MaaResult};

/// Get the installation directory for MaaFramework
pub fn get_install_dir() -> PathBuf {
    std::env::var("MAA_INSTALL_DIR")
        .map(PathBuf::from)
        .unwrap_or_else(|_| PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("MAA-win-x86_64-v5.4.1"))
}

/// Get the test resources directory (test/TestingDataSet/PipelineSmoking)
///
/// Discovery order:
/// 1. `MAA_TEST_RESOURCES_DIR` environment variable (for CI overrides)
/// 2. Walk up from CARGO_MANIFEST_DIR to find repo root (contains `.git` or `CMakeLists.txt`)
pub fn get_test_resources_dir() -> PathBuf {
    if let Ok(dir) = std::env::var("MAA_TEST_RESOURCES_DIR") {
        let path = PathBuf::from(&dir);
        if path.exists() {
            return path;
        }
    }

    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));

    let mut current = manifest_dir.as_path();
    while let Some(parent) = current.parent() {
        let git_dir = parent.join(".git");
        let cmake_file = parent.join("CMakeLists.txt");

        if git_dir.exists() || cmake_file.exists() {
            let test_path = parent
                .join("test")
                .join("TestingDataSet")
                .join("PipelineSmoking");
            if test_path.exists() {
                return test_path.canonicalize().unwrap_or(test_path);
            }
        }
        current = parent;
    }

    panic!("Test resources not found.");
}

/// Initialize the test environment with logging
pub fn init_test_env() -> MaaResult<()> {
    // Set environment variable for Toolkit to find binary
    let install_dir = get_install_dir();
    let bin_dir = install_dir.join("bin");

    // We try to init Toolkit option, but if it fails (already inited), we ignore
    let _ = Toolkit::init_option(bin_dir.to_str().unwrap(), "{}");

    maa_framework::set_debug_mode(true)?;
    maa_framework::set_stdout_level(sys::MaaLoggingLevelEnum_MaaLoggingLevel_All as i32)?;

    let log_dir = std::env::temp_dir().join("maa_test_logs");
    std::fs::create_dir_all(&log_dir).ok();
    maa_framework::configure_logging(log_dir.to_str().unwrap())?;

    Ok(())
}

/// ImageController - exactly like Python's DbgController with CarouselImage
/// Used for feeding images from the test dataset
pub struct ImageController {
    images: Vec<PathBuf>,
    index: AtomicUsize,
}

impl ImageController {
    pub fn new(dir: PathBuf) -> Self {
        let mut images = vec![];
        if let Ok(entries) = std::fs::read_dir(&dir) {
            for entry in entries.flatten() {
                let path = entry.path();
                if path.extension().map_or(false, |e| e == "png" || e == "jpg") {
                    images.push(path);
                }
            }
        }
        images.sort();
        println!(
            "ImageController loaded {} images from {:?}",
            images.len(),
            dir
        );
        Self {
            images,
            index: AtomicUsize::new(0),
        }
    }
}

impl CustomControllerCallback for ImageController {
    fn connect(&self) -> bool {
        true
    }

    fn request_uuid(&self) -> Option<String> {
        Some("ImageControllerUUID".to_string())
    }

    fn screencap(&self) -> Option<Vec<u8>> {
        if self.images.is_empty() {
            return None;
        }
        let idx = self.index.load(Ordering::SeqCst) % self.images.len();
        self.index.fetch_add(1, Ordering::SeqCst);

        let path = &self.images[idx];
        match std::fs::read(path) {
            Ok(data) => Some(data),
            Err(e) => {
                println!("Failed to read image {:?}: {}", path, e);
                None
            }
        }
    }

    fn click(&self, _x: i32, _y: i32) -> bool {
        true
    }
    fn swipe(&self, _x1: i32, _y1: i32, _x2: i32, _y2: i32, _duration: i32) -> bool {
        true
    }
    fn touch_down(&self, _contact: i32, _x: i32, _y: i32, _pressure: i32) -> bool {
        true
    }
    fn touch_move(&self, _contact: i32, _x: i32, _y: i32, _pressure: i32) -> bool {
        true
    }
    fn touch_up(&self, _contact: i32) -> bool {
        true
    }
    fn click_key(&self, _keycode: i32) -> bool {
        true
    }
    fn input_text(&self, _text: &str) -> bool {
        true
    }
    fn key_down(&self, _keycode: i32) -> bool {
        true
    }
    fn key_up(&self, _keycode: i32) -> bool {
        true
    }
    fn scroll(&self, _dx: i32, _dy: i32) -> bool {
        true
    }
}
