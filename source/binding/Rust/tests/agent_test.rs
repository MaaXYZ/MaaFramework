//! Agent IPC integration test using multi-process client/server architecture.

mod common;

use std::env;
use std::process::{Command, Stdio};

use std::thread;
use std::time::Duration;

use maa_framework::agent_client::AgentClient;
use maa_framework::agent_server::AgentServer;
use maa_framework::context::Context;
use maa_framework::controller::Controller;
use maa_framework::custom::{CustomAction, CustomRecognition};
use maa_framework::resource::Resource;
use maa_framework::tasker::Tasker;
use maa_framework::{self, sys};

use common::{get_test_resources_dir, init_test_env, ImageController};

struct ServerRecognition;

impl CustomRecognition for ServerRecognition {
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
            "[Server] MyRecognition analyze called! node: {}, reco: {}",
            node_name, custom_recognition_name
        );

        // Test Context API - Ported from binding_test.rs
        let ppover = r#"{
            "ColorMatch": {
                "recognition": "ColorMatch",
                "lower": [100, 100, 100],
                "upper": [255, 255, 255],
                "action": "Click"
            }
        }"#;
        let run_result = context.run_task("ColorMatch", ppover);
        println!("  [Server] run_task result: {:?}", run_result);

        if let Ok(new_ctx) = context.clone_context() {
            println!("  [Server] clone_context: OK");

            // Test override_pipeline
            let override_result = new_ctx.override_pipeline(r#"{"TaskA": {}, "TaskB": {}}"#);
            println!("  [Server] override_pipeline result: {:?}", override_result);

            // Test get_node_data (Context level)
            let node_data = new_ctx.get_node_data(node_name);
            println!(
                "  [Server] get_node_data({}) is_some: {}",
                node_name,
                node_data.as_ref().map_or(false, |o| o.is_some())
            );

            // Test anchor API
            let set_result = new_ctx.set_anchor("test_anchor", "TaskA");
            println!("  [Server] set_anchor result: {:?}", set_result);

            let anchor = new_ctx.get_anchor("test_anchor");
            println!("  [Server] get_anchor result: {:?}", anchor);

            // Test hit count API
            let hit_count = new_ctx.get_hit_count(node_name);
            println!("  [Server] get_hit_count({}) = {:?}", node_name, hit_count);

            let clear_result = new_ctx.clear_hit_count(node_name);
            println!("  [Server] clear_hit_count result: {:?}", clear_result);

            // Test get_task_job
            let task_job = new_ctx.get_task_job();
            let task_detail = task_job.get(false);
            println!(
                "  [Server] get_task_job entry: {:?}",
                task_detail
                    .as_ref()
                    .ok()
                    .and_then(|o| o.as_ref().map(|d| d.entry.clone()))
            );
        }

        // 3. Test task_id
        let task_id = context.task_id();
        println!("  [Server] context.task_id: {}", task_id);

        Some((
            sys::MaaRect {
                x: 10,
                y: 20,
                width: 100,
                height: 200,
            },
            "server_verified".to_string(), // Signal to client
        ))
    }
}

struct ServerAction;

impl CustomAction for ServerAction {
    fn run(
        &self,
        _context: &Context,
        _task_id: sys::MaaTaskId,
        _node_name: &str,
        _custom_action_name: &str,
        _custom_action_param: &str,
        _reco_id: sys::MaaRecoId,
        _box_rect: &sys::MaaRect,
    ) -> bool {
        println!("[Server] MyAction run called!");
        true
    }
}

fn agent_server_main() {
    println!("[Server] Starting...");
    let identifier = "MaaAgentTest_MultiProcess";

    AgentServer::start_up(identifier).expect("Failed to start AgentServer");

    AgentServer::register_custom_recognition("MyRec", Box::new(ServerRecognition)).unwrap();
    AgentServer::register_custom_action("MyAct", Box::new(ServerAction)).unwrap();

    println!("[Server] Running. Waiting for client...");

    loop {
        thread::sleep(Duration::from_millis(100));
    }
}

#[test]
fn test_agent_full_integration() {
    if env::var("MAA_AGENT_TEST_MODE").unwrap_or_default() == "SERVER" {
        agent_server_main();
        return;
    }

    println!("\n=== test_agent_full_integration (Client) ===");
    init_test_env().unwrap();
    let current_exe = env::current_exe().expect("Failed to get current exe path");
    println!("Spawning server: {:?}", current_exe);

    let mut server_process = Command::new(&current_exe)
        .arg("test_agent_full_integration")
        .arg("--nocapture")
        .env("MAA_AGENT_TEST_MODE", "SERVER")
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .expect("Failed to spawn server process");

    // Waiting for server to start
    thread::sleep(Duration::from_millis(500));

    let identifier = "MaaAgentTest_MultiProcess";

    let client = AgentClient::new(Some(identifier)).expect("Failed to create AgentClient");

    let res_dir = get_test_resources_dir();
    let resource_dir = res_dir.join("resource");
    let screenshot_dir = res_dir.join("Screenshot");
    assert!(resource_dir.exists());

    let resource = Resource::new().unwrap();
    let id = resource
        .post_bundle(resource_dir.to_str().unwrap())
        .unwrap();
    resource.wait(id);
    client.bind(&resource).expect("Failed to bind resource");

    let img_ctrl = ImageController::new(screenshot_dir);
    let controller = Controller::new_custom(img_ctrl).unwrap();
    let conn_id = controller.post_connection().unwrap();
    controller.wait(conn_id);

    println!("Connecting to agent...");

    // Robust connection loop for CI stability
    let mut connected = false;
    for i in 0..20 {
        // Retry ~10 seconds
        if client.connect().is_ok() {
            connected = true;
            println!("Connected on attempt {}", i + 1);
            break;
        }
        thread::sleep(Duration::from_millis(500));
    }

    if !connected {
        // Kill server if connect fails
        let _ = server_process.kill();
        panic!("Failed to connect to AgentServer after retries");
    }

    assert!(client.connected(), "Agent must be connected");

    let tasker = Tasker::new().unwrap();
    tasker.bind_resource(&resource).unwrap();
    tasker.bind_controller(&controller).unwrap();

    let ppover = r#"{
        "TestEntry": {"next": ["TestNode"]},
        "TestNode": {
            "recognition": "Custom",
            "custom_recognition": "MyRec",
            "action": "Custom",
            "custom_action": "MyAct"
        }
    }"#;

    let job = tasker
        .post_task("TestEntry", ppover)
        .expect("post_task failed");
    let status = job.wait();
    println!("Task status: {:?}", status);

    // 8. IPC Check
    let detail = job.get(true).expect("Task failed to produce detail");
    let detail_str = format!("{:?}", detail);
    println!("Task Detail: {}", detail_str);

    let mut recognition_detail_found = false;

    if let Some(d) = detail {
        println!("Iterating node_id_list: {:?}", d.node_id_list);
        for &node_id in &d.node_id_list {
            match tasker.get_node_detail(node_id) {
                Ok(Some(node_detail)) => {
                    println!("Node Detail [{}]: {:?}", node_id, node_detail);
                    if node_detail.reco_id > 0 {
                        match tasker.get_recognition_detail(node_detail.reco_id) {
                            Ok(Some(reco_detail)) => {
                                println!(
                                    "Reco Detail [{}]: {:?}",
                                    node_detail.reco_id, reco_detail
                                );
                                let json_str = reco_detail.detail.to_string();
                                println!("Reco JSON: {}", json_str);

                                if json_str.contains("server_verified") {
                                    recognition_detail_found = true;
                                    break;
                                }
                            }
                            Ok(None) => println!("Reco Detail [{}] is None", node_detail.reco_id),
                            Err(e) => println!("Error getting reco detail: {:?}", e),
                        }
                    } else {
                        println!("Node [{}] has no reco_id", node_id);
                    }
                }
                Ok(None) => println!("Node Detail [{}] is None", node_id),
                Err(e) => println!("Error getting node detail: {:?}", e),
            }
        }
    } else {
        println!("Task detail is None");
    }

    // Cleanup
    client.disconnect().unwrap();
    let _ = server_process.kill();

    if !status.succeeded() {
        panic!("Task failed execution");
    }

    if !recognition_detail_found {
        panic!("IPC Verification Failed: Did not find 'server_verified' in recognition details. Task Detail: {}", detail_str);
    }

    println!("PASS: agent multi-process integration");
}
