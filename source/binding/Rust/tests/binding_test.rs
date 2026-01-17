//! Comprehensive binding tests matching Python binding_test.py
//!
//! Test coverage:
//! - Resource: loading, inference settings, custom registration, event sinks
//! - Controller: DbgController, connection, input operations, screenshots
//! - Tasker: task execution, status queries, detail retrieval, event sinks
//! - CustomController: custom controller implementation
//! - Toolkit: device discovery

mod common;

use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};
use std::sync::Arc;

use maa_framework::context::Context;
use maa_framework::controller::Controller;
use maa_framework::custom::{CustomAction, CustomRecognition};
use maa_framework::custom_controller::CustomControllerCallback;
use maa_framework::resource::Resource;
use maa_framework::tasker::Tasker;
use maa_framework::toolkit::Toolkit;
use maa_framework::{self, sys};

use common::{get_test_resources_dir, init_test_env};

// ============================================================================
// Custom Recognition/Action for testing
// ============================================================================

static ANALYZED: AtomicBool = AtomicBool::new(false);
static RUNNED: AtomicBool = AtomicBool::new(false);

struct MyRecognition;

impl CustomRecognition for MyRecognition {
    fn analyze(
        &self,
        context: &Context,
        _task_id: sys::MaaTaskId,
        node_name: &str,
        custom_recognition_name: &str,
        _custom_recognition_param: &str,
        _image: *const sys::MaaImageBuffer,
        _roi: &sys::MaaRect,
    ) -> Option<(sys::MaaRect, String)> {
        println!(
            "on MyRecognition.analyze, context: {:?}, node: {}, reco: {}",
            context.raw(),
            node_name,
            custom_recognition_name
        );

        // ================================================================
        // Test Context API - matching Python's binding_test.py coverage
        // ================================================================

        // 1. Test run_task
        let ppover = r#"{
            "ColorMatch": {
                "recognition": "ColorMatch",
                "lower": [100, 100, 100],
                "upper": [255, 255, 255],
                "action": "Click"
            }
        }"#;
        let run_result = context.run_task("ColorMatch", ppover);
        println!("  run_task result: {:?}", run_result);

        // 3. Test run_action and run_recognition
        let rect = maa_framework::common::Rect {
            x: 114,
            y: 514,
            width: 191,
            height: 810,
        };
        let action_detail = context.run_action("ColorMatch", ppover, &rect, "RunAction Detail");
        println!("  run_action result: {:?}", action_detail);

        // Use a dummy buffer for testing API
        let dummy_img = maa_framework::buffer::MaaImageBuffer::new();
        if let Ok(img) = dummy_img {
            let reco_detail = context.run_recognition("ColorMatch", ppover, &img);
            println!("  run_recognition result: {:?}", reco_detail);
        }

        // 4. Test clone_context and override_pipeline
        if let Ok(new_ctx) = context.clone_context() {
            println!("  clone_context: OK");

            // Test override_pipeline
            let override_result = new_ctx.override_pipeline(r#"{"TaskA": {}, "TaskB": {}}"#);
            println!("  override_pipeline result: {:?}", override_result);

            // Test override_next
            let override_next = new_ctx.override_next(node_name, &["TaskA", "TaskB"]);
            println!("  override_next result: {:?}", override_next);

            // Test get_node_data (Context level)
            let node_data = new_ctx.get_node_data(node_name);
            println!(
                "  get_node_data({}) is_some: {}",
                node_name,
                node_data.as_ref().map_or(false, |o| o.is_some())
            );

            // Test anchor API
            let set_result = new_ctx.set_anchor("test_anchor", "TaskA");
            println!("  set_anchor result: {:?}", set_result);

            let anchor = new_ctx.get_anchor("test_anchor");
            println!("  get_anchor result: {:?}", anchor);

            // Test hit count API
            let hit_count = new_ctx.get_hit_count(node_name);
            println!("  get_hit_count({}) = {:?}", node_name, hit_count);

            let clear_result = new_ctx.clear_hit_count(node_name);
            println!("  clear_hit_count result: {:?}", clear_result);

            // Test override_image
            let mut test_image_buf = maa_framework::buffer::MaaImageBuffer::new()
                .expect("Failed to create image buffer");
            let raw_data = vec![0u8; 100 * 100 * 3];
            let _ = test_image_buf.set_raw_data(&raw_data, 100, 100, 16); // 16 = CV_8UC3
            let override_img = new_ctx.override_image("test_image", &test_image_buf);
            println!("  override_image result: {:?}", override_img);

            // Test from tasker - NOT SUPPORTED
            // if let Some(tasker) = new_ctx.tasker() { ... }

            // Test get_task_job
            let task_job = new_ctx.get_task_job();
            let task_detail = task_job.get(false);
            println!(
                "  get_task_job entry: {:?}",
                task_detail
                    .as_ref()
                    .ok()
                    .and_then(|o| o.as_ref().map(|d| d.entry.clone()))
            );
        }

        // 5. Test task_id
        let task_id = context.task_id();
        println!("  context.task_id: {}", task_id);

        ANALYZED.store(true, Ordering::SeqCst);

        Some((
            sys::MaaRect {
                x: 11,
                y: 4,
                width: 5,
                height: 14,
            },
            r#"{"message": "Hello World!"}"#.to_string(),
        ))
    }
}

struct MyAction;

impl CustomAction for MyAction {
    fn run(
        &self,
        context: &Context,
        _task_id: sys::MaaTaskId,
        node_name: &str,
        custom_action_name: &str,
        _custom_action_param: &str,
        _reco_id: sys::MaaRecoId,
        _box_rect: &sys::MaaRect,
    ) -> bool {
        println!(
            "on MyAction.run, context: {:?}, node: {}, action: {}",
            context.raw(),
            node_name,
            custom_action_name
        );

        RUNNED.store(true, Ordering::SeqCst);

        true
    }
}

// ============================================================================
// Custom Controller for testing
// ============================================================================

struct MyController {
    count: Arc<AtomicUsize>,
    image: Vec<u8>,
}

impl MyController {
    fn new(count: Arc<AtomicUsize>) -> Self {
        let test_res_dir = common::get_test_resources_dir();
        let screenshot_dir = test_res_dir.join("Screenshot");

        let mut image = Vec::new();
        if let Ok(entries) = std::fs::read_dir(screenshot_dir) {
            for entry in entries.flatten() {
                let path = entry.path();
                if path.extension().map_or(false, |e| e == "png") {
                    if let Ok(data) = std::fs::read(path) {
                        image = data;
                        break;
                    }
                }
            }
        }

        if image.is_empty() {
            println!("WARNING: No PNG found in Screenshot dir, using empty vec (will fail task)");
        }

        Self { count, image }
    }
}

impl CustomControllerCallback for MyController {
    fn connect(&self) -> bool {
        println!("  on MyController.connect");
        self.count.fetch_add(1, Ordering::SeqCst);
        true
    }

    fn request_uuid(&self) -> Option<String> {
        println!("  on MyController.request_uuid");
        Some("12345678".to_string())
    }

    fn start_app(&self, intent: &str) -> bool {
        println!("  on MyController.start_app: {}", intent);
        self.count.fetch_add(1, Ordering::SeqCst);
        true
    }

    fn stop_app(&self, intent: &str) -> bool {
        println!("  on MyController.stop_app: {}", intent);
        self.count.fetch_add(1, Ordering::SeqCst);
        true
    }

    fn screencap(&self) -> Option<Vec<u8>> {
        println!("  on MyController.screencap");
        self.count.fetch_add(1, Ordering::SeqCst);
        if self.image.is_empty() {
            Some(Vec::new())
        } else {
            Some(self.image.clone())
        }
    }

    fn click(&self, x: i32, y: i32) -> bool {
        println!("  on MyController.click: {}, {}", x, y);
        self.count.fetch_add(1, Ordering::SeqCst);
        true
    }

    fn swipe(&self, x1: i32, y1: i32, x2: i32, y2: i32, duration: i32) -> bool {
        println!(
            "  on MyController.swipe: {}, {} -> {}, {}, {}",
            x1, y1, x2, y2, duration
        );
        self.count.fetch_add(1, Ordering::SeqCst);
        true
    }

    fn touch_down(&self, contact: i32, x: i32, y: i32, pressure: i32) -> bool {
        println!(
            "  on MyController.touch_down: {}, {}, {}, {}",
            contact, x, y, pressure
        );
        self.count.fetch_add(1, Ordering::SeqCst);
        true
    }

    fn touch_move(&self, contact: i32, x: i32, y: i32, pressure: i32) -> bool {
        println!(
            "  on MyController.touch_move: {}, {}, {}, {}",
            contact, x, y, pressure
        );
        self.count.fetch_add(1, Ordering::SeqCst);
        true
    }

    fn touch_up(&self, contact: i32) -> bool {
        println!("  on MyController.touch_up: {}", contact);
        self.count.fetch_add(1, Ordering::SeqCst);
        true
    }

    fn click_key(&self, keycode: i32) -> bool {
        println!("  on MyController.click_key: {}", keycode);
        self.count.fetch_add(1, Ordering::SeqCst);
        true
    }

    fn input_text(&self, text: &str) -> bool {
        println!("  on MyController.input_text: {}", text);
        self.count.fetch_add(1, Ordering::SeqCst);
        true
    }

    fn key_down(&self, keycode: i32) -> bool {
        println!("  on MyController.key_down: {}", keycode);
        self.count.fetch_add(1, Ordering::SeqCst);
        true
    }

    fn key_up(&self, keycode: i32) -> bool {
        println!("  on MyController.key_up: {}", keycode);
        self.count.fetch_add(1, Ordering::SeqCst);
        true
    }

    fn scroll(&self, dx: i32, dy: i32) -> bool {
        println!("  on MyController.scroll: {}, {}", dx, dy);
        self.count.fetch_add(1, Ordering::SeqCst);
        true
    }
}

// ============================================================================
// Resource API Tests
// ============================================================================

#[test]
fn test_resource_inference_settings() {
    println!("\n=== test_resource_inference_settings ===");

    let r1 = Resource::new().expect("Failed to create resource");
    // Test inference device settings (should not panic)
    let _ = r1.use_cpu();
    let _ = r1.use_directml(0);
    let _ = r1.use_auto_ep();

    println!("  PASS: inference settings");
}

#[test]
fn test_resource_loading() {
    println!("\n=== test_resource_loading ===");

    let _ = init_test_env();

    let resource = Resource::new().expect("Failed to create resource");

    // Test invalid path - wait for task completion to verify no crash
    if let Ok(id) = resource.post_bundle("C:/_maafw_testing_/aaabbbccc") {
        let status = resource.wait(id);
        println!("  invalid path status: {:?} (expected failure)", status);
    }
    println!(
        "  resource.loaded (after invalid path): {}",
        resource.loaded()
    );

    // Test loading valid resources - STRICT: resources MUST exist (git submodule)
    let test_res_dir = get_test_resources_dir();
    let resource_path = test_res_dir.join("resource");

    assert!(
        resource_path.exists(),
        "Test resources MUST exist at {:?}. Run: git submodule update --init",
        resource_path
    );

    let id = resource
        .post_bundle(resource_path.to_str().unwrap())
        .expect("post_bundle MUST succeed");
    let status = resource.wait(id);
    assert!(status.done(), "Resource loading MUST complete");
    assert!(resource.loaded(), "Resource MUST be loaded");
    println!(
        "  resource.loaded: {}, status: {:?}",
        resource.loaded(),
        status
    );

    println!("  PASS: resource loading (STRICT)");
}

#[test]
fn test_resource_custom_registration() {
    println!("\n=== test_resource_custom_registration ===");

    let resource = Resource::new().expect("Failed to create resource");

    // Register custom recognition and action
    resource
        .register_custom_recognition("MyRec", Box::new(MyRecognition))
        .expect("Failed to register recognition");
    resource
        .register_custom_action("MyAct", Box::new(MyAction))
        .expect("Failed to register action");

    // Test custom lists
    let reco_list = resource
        .custom_recognition_list()
        .expect("Failed to get reco list");
    let action_list = resource
        .custom_action_list()
        .expect("Failed to get action list");
    println!("  custom_recognition_list: {:?}", reco_list);
    println!("  custom_action_list: {:?}", action_list);

    assert!(
        reco_list.contains(&"MyRec".to_string()),
        "MyRec should be registered"
    );
    assert!(
        action_list.contains(&"MyAct".to_string()),
        "MyAct should be registered"
    );

    // Test unregister
    resource
        .unregister_custom_recognition("MyRec")
        .expect("Failed to unregister recognition");
    resource
        .unregister_custom_action("MyAct")
        .expect("Failed to unregister action");

    let reco_list_after = resource
        .custom_recognition_list()
        .expect("Failed to get reco list");
    let action_list_after = resource
        .custom_action_list()
        .expect("Failed to get action list");
    assert!(
        !reco_list_after.contains(&"MyRec".to_string()),
        "MyRec should be unregistered"
    );
    assert!(
        !action_list_after.contains(&"MyAct".to_string()),
        "MyAct should be unregistered"
    );

    // Test clear
    resource
        .register_custom_recognition("MyRec2", Box::new(MyRecognition))
        .expect("Failed to register recognition");
    resource
        .clear_custom_recognition()
        .expect("Failed to clear");
    let reco_list_cleared = resource
        .custom_recognition_list()
        .expect("Failed to get reco list");
    assert!(reco_list_cleared.is_empty(), "Should be empty after clear");

    println!("  PASS: custom registration");
}

#[test]
fn test_resource_sink_operations() {
    println!("\n=== test_resource_sink_operations ===");

    let resource = Resource::new().expect("Failed to create resource");

    // Add sink
    let sink_id = resource
        .add_sink(|msg, details| {
            println!("  [ResourceSink] msg: {}, details: {}", msg, details);
        })
        .expect("Failed to add sink");
    println!("  sink_id: {:?}", sink_id);

    // Remove sink
    resource.remove_sink(sink_id);

    // Clear sinks
    let _ = resource.add_sink(|_, _| {});
    resource.clear_sinks();

    println!("  PASS: sink operations");
}

#[test]
fn test_resource_node_operations() {
    println!("\n=== test_resource_node_operations ===");

    let _ = init_test_env();

    let resource = Resource::new().expect("Failed to create resource");

    // Load test resources - STRICT: MUST exist
    let test_res_dir = get_test_resources_dir();
    let resource_path = test_res_dir.join("resource");

    assert!(
        resource_path.exists(),
        "Test resources MUST exist at {:?}. Run: git submodule update --init",
        resource_path
    );

    let id = resource
        .post_bundle(resource_path.to_str().unwrap())
        .expect("post_bundle MUST succeed");
    resource.wait(id);

    assert!(resource.loaded(), "Resource MUST be loaded");

    // Test node_list - STRICT
    let node_list = resource.node_list().expect("node_list MUST work");
    println!("  node_list count: {}", node_list.len());
    assert!(!node_list.is_empty(), "node_list MUST NOT be empty");

    // Test get_node_data - STRICT
    let first_node = node_list
        .first()
        .expect("Node list MUST have at least one node");
    let node_data = resource
        .get_node_data(first_node)
        .expect("get_node_data MUST NOT error")
        .expect("get_node_data MUST return Some");
    assert!(
        node_data.contains("recognition"),
        "node_data MUST contain 'recognition'"
    );
    println!(
        "  get_node_data({}) verified: {} bytes",
        first_node,
        node_data.len()
    );

    // Test hash - STRICT
    let hash = resource.hash().expect("hash MUST work");
    assert!(!hash.is_empty(), "Hash MUST NOT be empty");
    println!("  resource.hash: {}", hash);

    println!("  PASS: node operations (STRICT)");
}

// ============================================================================
// Controller API Tests
// ============================================================================

#[test]
fn test_dbg_controller_creation() {
    println!("\n=== test_dbg_controller_creation ===");

    let test_res_dir = get_test_resources_dir();
    let screenshot_dir = test_res_dir.join("Screenshot");

    if !screenshot_dir.exists() {
        panic!(
            "Screenshot directory not found at {:?}. Run: git submodule update --init",
            screenshot_dir
        );
    }

    // Create a user dir in temp
    let user_dir = std::env::temp_dir().join("maa_test_user");
    std::fs::create_dir_all(&user_dir).ok();

    let controller = Controller::new_dbg(
        screenshot_dir.to_str().unwrap(),
        user_dir.to_str().unwrap(),
        sys::MaaDbgControllerType_CarouselImage as u64,
        "{}",
    );

    match controller {
        Ok(_) => println!("  PASS: DbgController creation"),
        Err(e) => {
            // MaaDbgControlUnit.dll is not included in release packages
            panic!(
                "DbgController not available ({}). Ensure MaaDbgControlUnit.dll is present.",
                e
            );
        }
    }
}

#[test]
fn test_controller_connection() {
    println!("\n=== test_controller_connection ===");

    let _ = init_test_env();

    let test_res_dir = get_test_resources_dir();
    let screenshot_dir = test_res_dir.join("Screenshot");

    if !screenshot_dir.exists() {
        panic!(
            "Screenshot directory not found at {:?}. Run: git submodule update --init",
            screenshot_dir
        );
    }

    let user_dir = std::env::temp_dir().join("maa_test_user");
    std::fs::create_dir_all(&user_dir).ok();

    let controller = match Controller::new_dbg(
        screenshot_dir.to_str().unwrap(),
        user_dir.to_str().unwrap(),
        sys::MaaDbgControllerType_CarouselImage as u64,
        "{}",
    ) {
        Ok(c) => c,
        Err(e) => {
            panic!(
                "DbgController not available ({}). Ensure MaaDbgControlUnit.dll is present.",
                e
            );
        }
    };

    // Test connection
    let conn_id = controller
        .post_connection()
        .expect("Failed to post connection");
    let status = controller.wait(conn_id);
    println!("  connection status: {:?}", status);
    println!("  connected: {}", controller.connected());

    // Test UUID
    let uuid = controller.uuid();
    println!("  uuid: {:?}", uuid);

    println!("  PASS: controller connection");
}

#[test]
fn test_controller_screencap() {
    println!("\n=== test_controller_screencap ===");

    let _ = init_test_env();

    let test_res_dir = get_test_resources_dir();
    let screenshot_dir = test_res_dir.join("Screenshot");

    if !screenshot_dir.exists() {
        panic!(
            "Screenshot directory not found at {:?}. Run: git submodule update --init",
            screenshot_dir
        );
    }

    let user_dir = std::env::temp_dir().join("maa_test_user");
    std::fs::create_dir_all(&user_dir).ok();

    let controller = match Controller::new_dbg(
        screenshot_dir.to_str().unwrap(),
        user_dir.to_str().unwrap(),
        sys::MaaDbgControllerType_CarouselImage as u64,
        "{}",
    ) {
        Ok(c) => c,
        Err(e) => {
            panic!(
                "DbgController not available ({}). Ensure MaaDbgControlUnit.dll is present.",
                e
            );
        }
    };

    // Connect first
    let conn_id = controller
        .post_connection()
        .expect("Failed to post connection");
    controller.wait(conn_id);

    // Test screencap
    let cap_id = controller
        .post_screencap()
        .expect("Failed to post screencap");
    let status = controller.wait(cap_id);
    println!("  screencap status: {:?}", status);

    // Test resolution
    let resolution = controller.resolution();
    println!("  resolution: {:?}", resolution);

    println!("  PASS: controller screencap");
}

#[test]
fn test_controller_sink_operations() {
    println!("\n=== test_controller_sink_operations ===");

    let test_res_dir = get_test_resources_dir();
    let screenshot_dir = test_res_dir.join("Screenshot");

    if !screenshot_dir.exists() {
        panic!(
            "Screenshot directory not found at {:?}. Run: git submodule update --init",
            screenshot_dir
        );
    }

    let user_dir = std::env::temp_dir().join("maa_test_user");
    std::fs::create_dir_all(&user_dir).ok();

    let controller = match Controller::new_dbg(
        screenshot_dir.to_str().unwrap(),
        user_dir.to_str().unwrap(),
        sys::MaaDbgControllerType_CarouselImage as u64,
        "{}",
    ) {
        Ok(c) => c,
        Err(e) => {
            panic!(
                "DbgController not available ({}). Ensure MaaDbgControlUnit.dll is present.",
                e
            );
        }
    };

    // Add sink
    let sink_id = controller
        .add_sink(|msg, details| {
            println!("  [ControllerSink] msg: {}, details: {}", msg, details);
        })
        .expect("Failed to add sink");
    println!("  sink_id: {:?}", sink_id);

    // Remove and clear
    controller.remove_sink(sink_id);
    let _ = controller.add_sink(|_, _| {});
    controller.clear_sinks();

    println!("  PASS: controller sink operations");
}

// ============================================================================
// Tasker API Tests
// ============================================================================

#[test]
fn test_tasker_global_options() {
    println!("\n=== test_tasker_global_options ===");

    // Test global options (static methods equivalent)
    maa_framework::set_save_draw(true).expect("Failed to set save_draw");
    maa_framework::set_stdout_level(sys::MaaLoggingLevelEnum_MaaLoggingLevel_All as i32)
        .expect("Failed to set stdout level");
    maa_framework::configure_logging(".").expect("Failed to configure logging");
    maa_framework::set_debug_mode(true).expect("Failed to set debug mode");
    maa_framework::set_save_on_error(true).expect("Failed to set save on error");
    maa_framework::set_draw_quality(85).expect("Failed to set draw quality");
    maa_framework::set_reco_image_cache_limit(4096).expect("Failed to set reco cache limit");

    println!("  PASS: global options");
}

#[test]
fn test_tasker_api() {
    println!("\n=== test_tasker_api ===");

    let _ = init_test_env();

    // Create tasker
    let tasker = Tasker::new().expect("Failed to create tasker");
    println!("  tasker created");

    // Create resource
    let resource = Resource::new().expect("Failed to create resource");

    // Load test resources
    let test_res_dir = get_test_resources_dir();
    let resource_path = test_res_dir.join("resource");
    assert!(
        resource_path.exists(),
        "Test resources MUST exist at {:?}. Run: git submodule update --init",
        resource_path
    );

    let id = resource
        .post_bundle(resource_path.to_str().unwrap())
        .expect("post_bundle MUST succeed");
    let status = resource.wait(id);
    assert!(status.succeeded(), "Resource loading failed");

    // Register custom components
    resource
        .register_custom_recognition("MyRec", Box::new(MyRecognition))
        .expect("Failed to register MyRec");
    resource
        .register_custom_action("MyAct", Box::new(MyAction))
        .expect("Failed to register MyAct");

    // Create controller (Use MyController for reliable testing without external DLLs)
    let count = Arc::new(AtomicUsize::new(0));
    let my_ctrl = MyController::new(count.clone());
    let controller = Controller::new_custom(my_ctrl).expect("Failed to create custom controller");

    // Connect
    let conn_id = controller.post_connection().unwrap();
    controller.wait(conn_id);

    // Bind
    tasker
        .bind_resource(&resource)
        .expect("Failed to bind resource");
    tasker
        .bind_controller(&controller)
        .expect("Failed to bind controller");

    println!("  inited: {}", tasker.inited());

    // Execute Task to trigger callbacks
    let ppover = r#"{
        "Entry": {"next": "Rec"},
        "Rec": {
            "recognition": "Custom",
            "custom_recognition": "MyRec",
            "action": "Custom",
            "custom_action": "MyAct",
            "custom_action_param": "Test111222333"
        }
    }"#;

    let task_job = tasker.post_task("Entry", ppover).expect("post_task failed");
    let task_status = task_job.wait();
    println!("  task status: {:?}", task_status);

    // Strict checks for callbacks
    let analyzed = ANALYZED.load(Ordering::SeqCst);
    let runned = RUNNED.load(Ordering::SeqCst);
    println!("  ANALYZED: {}, RUNNED: {}", analyzed, runned);

    assert!(analyzed, "MyRecognition.analyze MUST be called");
    assert!(runned, "MyAction.run MUST be called");

    println!("  PASS: tasker API (Active)");
}

#[test]
fn test_tasker_sink_operations() {
    println!("\n=== test_tasker_sink_operations ===");

    let tasker = Tasker::new().expect("Failed to create tasker");

    // Add tasker sink
    let tasker_sink_id = tasker
        .add_sink(|msg, details| {
            println!("  [TaskerSink] msg: {}, details: {}", msg, details);
        })
        .expect("Failed to add tasker sink");

    // Add context sink
    let context_sink_id = tasker
        .add_context_sink(|msg, details| {
            println!("  [ContextSink] msg: {}, details: {}", msg, details);
        })
        .expect("Failed to add context sink");

    println!(
        "  tasker_sink_id: {:?}, context_sink_id: {:?}",
        tasker_sink_id, context_sink_id
    );

    // Remove sinks
    tasker.remove_sink(tasker_sink_id);
    tasker.remove_context_sink(context_sink_id);

    // Clear sinks
    let _ = tasker.add_sink(|_, _| {});
    let _ = tasker.add_context_sink(|_, _| {});
    tasker.clear_sinks();
    tasker.clear_context_sinks();

    println!("  PASS: tasker sink operations");
}

#[test]
fn test_tasker_state_queries() {
    println!("\n=== test_tasker_state_queries ===");

    let tasker = Tasker::new().expect("Failed to create tasker");

    println!("  running: {}", tasker.running());
    println!("  stopping: {}", tasker.stopping());

    // Test clear cache
    tasker.clear_cache().expect("Failed to clear cache");

    println!("  PASS: tasker state queries");
}

// ============================================================================
// CustomController Tests
// ============================================================================

#[test]
fn test_custom_controller_operations() {
    println!("\n=== test_custom_controller_operations ===");

    let count = Arc::new(AtomicUsize::new(0));
    let my_ctrl = MyController::new(count.clone());

    let controller = Controller::new_custom(my_ctrl).expect("Failed to create custom controller");

    // Test connection
    let conn_id = controller
        .post_connection()
        .expect("Failed to post connection");
    let status = controller.wait(conn_id);
    println!("  connection status: {:?}", status);

    // Test UUID
    let uuid = controller.uuid();
    println!("  uuid: {:?}", uuid);

    // Test various operations - wait for each to complete
    if let Ok(id) = controller.post_click(100, 200) {
        controller.wait(id);
    }
    if let Ok(id) = controller.post_swipe(100, 200, 300, 400, 200) {
        controller.wait(id);
    }
    if let Ok(id) = controller.post_touch_down(1, 100, 100, 0) {
        controller.wait(id);
    }
    if let Ok(id) = controller.post_touch_move(1, 200, 200, 0) {
        controller.wait(id);
    }
    if let Ok(id) = controller.post_touch_up(1) {
        controller.wait(id);
    }
    if let Ok(id) = controller.post_click_key(32) {
        controller.wait(id);
    }
    if let Ok(id) = controller.post_input_text("Hello World!") {
        controller.wait(id);
    }

    // Verify callbacks are executed
    let executed_count = count.load(Ordering::SeqCst);
    println!("  total callbacks executed: {}", executed_count);
    assert!(
        executed_count > 0,
        "Custom controller callbacks executed: {}",
        executed_count
    );

    println!("  PASS: custom controller operations");
}

// ============================================================================
// Toolkit Tests
// ============================================================================

#[test]
fn test_toolkit_find_devices() {
    println!("\n=== test_toolkit_find_devices ===");

    let _ = init_test_env();

    // Find ADB devices - may return empty or error, both are acceptable
    match Toolkit::find_adb_devices() {
        Ok(devices) => println!("  adb devices: {}", devices.len()),
        Err(e) => println!("  adb devices: error ({:?})", e),
    }

    // Find desktop windows - may not be implemented on all platforms
    // CI environments often don't have a desktop, so we just verify no crash
    match Toolkit::find_desktop_windows() {
        Ok(list) => {
            println!("  desktop windows: {}", list.len());
            for win in list.iter().take(3) {
                let name = if win.window_name.len() > 30 {
                    &win.window_name[..30]
                } else {
                    &win.window_name
                };
                println!("    - {}", name);
            }
        }
        Err(e) => {
            // Not implemented or no access to desktop - acceptable in CI
            println!("  desktop windows: unavailable ({:?})", e);
        }
    }

    println!("  PASS: toolkit API calls completed without crash");
}

// ============================================================================
// Version Test
// ============================================================================

#[test]
fn test_maa_version() {
    println!("\n=== test_maa_version ===");

    let version = maa_framework::maa_version();
    println!("  MaaFw Version: {}", version);
    assert!(!version.is_empty(), "Version should not be empty");

    println!("  PASS: version");
}

// ============================================================================
// Native Controller Integration Test
// ============================================================================

/// Test Tasker with native DbgController to verify C++ integration.
///
/// This complements test_tasker_api (which uses CustomController) by testing
/// the path where Rust drives a C++ native controller component.
#[test]
fn test_tasker_with_native_dbg_controller() {
    println!("\n=== test_tasker_with_native_dbg_controller ===");

    let _ = init_test_env();

    let test_res_dir = get_test_resources_dir();
    let screenshot_dir = test_res_dir.join("Screenshot");
    let resource_path = test_res_dir.join("resource");
    let user_dir = std::env::temp_dir().join("maa_test_user_native");
    std::fs::create_dir_all(&user_dir).ok();

    assert!(
        screenshot_dir.exists(),
        "Screenshot dir MUST exist at {:?}",
        screenshot_dir
    );

    // Create native DbgController
    let controller = match Controller::new_dbg(
        screenshot_dir.to_str().unwrap(),
        user_dir.to_str().unwrap(),
        sys::MaaDbgControllerType_CarouselImage as u64,
        "{}",
    ) {
        Ok(c) => c,
        Err(e) => {
            println!("  SKIPPED: MaaDbgControlUnit not available ({})", e);
            return;
        }
    };

    // Connect
    let conn_id = controller.post_connection().unwrap();
    let conn_status = controller.wait(conn_id);
    assert!(conn_status.succeeded(), "DbgController connection failed");
    println!("  DbgController connected");

    // Prepare Resource
    let resource = Resource::new().unwrap();
    let res_id = resource
        .post_bundle(resource_path.to_str().unwrap())
        .expect("post_bundle MUST succeed");
    let res_status = resource.wait(res_id);
    assert!(res_status.succeeded(), "Resource loading failed");

    // Create and bind Tasker
    let tasker = Tasker::new().unwrap();
    tasker.bind_resource(&resource).unwrap();
    tasker.bind_controller(&controller).unwrap();

    assert!(tasker.inited(), "Tasker must be initialized");
    println!("  Tasker initialized with native DbgController");

    // Run a simple task - just verify the path works
    let ppover = r#"{"TestEntry": {"action": "Click", "target": [10, 10, 20, 20]}}"#;
    let job = tasker.post_task("TestEntry", ppover).unwrap();
    let status = job.wait();

    println!("  Task status: {:?}", status);
    assert!(
        status.succeeded(),
        "Task with native DbgController should succeed"
    );

    println!("  PASS: tasker with native DbgController");
}
