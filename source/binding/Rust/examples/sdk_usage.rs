//! SDK usage example demonstrating pipeline construction and task execution.

use maa_framework::controller::Controller;
use maa_framework::pipeline::{
    Action, ActionParam, ActionType, PipelineData, Recognition, RecognitionParam, RecognitionType,
};
use maa_framework::resource::Resource;
use maa_framework::tasker::Tasker;
use maa_framework::toolkit::Toolkit;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Find connected devices
    println!("Scanning for devices...");
    let devices = Toolkit::find_adb_devices()?;
    for device in &devices {
        println!("Found: {} ({})", device.name, device.address);
    }

    if devices.is_empty() {
        println!("No devices found, skipping connection.");
    }

    // Construct pipeline with typed structs
    let my_task = PipelineData {
        recognition: Recognition {
            recognition_type: RecognitionType::DirectHit,
            param: RecognitionParam::default(),
        },
        action: Action {
            action_type: ActionType::InputText,
            param: ActionParam {
                input_text: Some("Hello World".to_string()),
                ..Default::default()
            },
        },
        next: vec![],
        rate_limit: 1000,
        timeout: 20000,
        on_error: vec![],
        anchor: vec![],
        inverse: false,
        enabled: true,
        pre_delay: 200,
        post_delay: 200,
        pre_wait_freezes: None,
        post_wait_freezes: None,
        repeat: 1,
        repeat_delay: 0,
        repeat_wait_freezes: None,
        max_hit: u32::MAX,
        focus: None,
        attach: Some(serde_json::Value::Null),
    };

    let pipeline_json = serde_json::to_string(&my_task).unwrap();
    println!("Generated Pipeline: {}", pipeline_json);

    let tasker = Tasker::new()?;
    let resource = Resource::new()?;
    tasker.bind_resource(&resource)?;
    #[cfg(feature = "adb")]
    if let Some(first_device) = devices.first() {
        let config_str = serde_json::to_string(&first_device.config)?;
        let controller = Controller::new_adb(
            first_device.adb_path.to_str().unwrap(),
            &first_device.address,
            &config_str,
            None,
        )?;
        tasker.bind_controller(&controller)?;

        let job = tasker.post_task("Start", &pipeline_json)?;
        println!("Job started, waiting...");
        let status = job.wait();
        println!("Job finished with status: {:?}", status);
        if let Ok(Some(detail)) = job.get(false) {
            println!("Task entry: {}", detail.entry);
            println!("Node count: {}", detail.node_id_list.len());
        }
    }

    Ok(())
}
