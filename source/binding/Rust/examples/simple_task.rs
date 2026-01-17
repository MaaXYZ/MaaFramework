//! Basic task execution example with event sink callbacks.

use maa_framework::controller::Controller;
use maa_framework::event_sink::{self, TaskerEventSink, TaskerTaskDetail};
use maa_framework::resource::Resource;
use maa_framework::tasker::Tasker;

struct MyTaskerSink;

impl event_sink::EventSink for MyTaskerSink {
    fn on_event(&self, msg: &str, details_json: &str) {
        // Fallback for unhandled events
        println!("  [Raw Event] {}: {}", msg, details_json);
    }
}

impl TaskerEventSink for MyTaskerSink {
    fn on_task_starting(&self, detail: TaskerTaskDetail) {
        println!(
            "Task Starting: ID={}, Entry={}",
            detail.task_id, detail.entry
        );
    }

    fn on_task_succeeded(&self, detail: TaskerTaskDetail) {
        println!("Task Succeeded: ID={}", detail.task_id);
    }

    fn on_task_failed(&self, detail: TaskerTaskDetail) {
        println!("Task Failed: ID={}", detail.task_id);
    }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("Initializing MaaFramework...");

    let tasker = Tasker::new()?;
    let _sink = MyTaskerSink;

    // Register event callback
    tasker.add_sink(|msg, _| {
        let notif_type = maa_framework::event_sink::parse_notification_type(msg);
        println!("Notification: {:?} - {}", notif_type, msg);
    })?;

    // Connect to ADB device
    #[cfg(feature = "adb")]
    {
        println!("Searching for devices...");
        let devices = maa_framework::toolkit::Toolkit::find_adb_devices()?;
        if let Some(device) = devices.first() {
            println!("Connecting to {}...", device.name);
            let adb_path = device.adb_path.to_string_lossy();
            let controller = Controller::new_adb(&adb_path, &device.address, "{}", None)?;
            tasker.bind_controller(&controller)?;
            println!("Controller connected.");
        } else {
            println!("No devices found, skipping controller binding.");
        }
    }

    // Load resources
    let resource = Resource::new()?;
    println!("Resource initialized.");
    tasker.bind_resource(&resource)?;

    if tasker.inited() {
        println!("Tasker initialized successfully.");
        // Run task: tasker.post_task("Start", "{}")?.wait();
    }

    Ok(())
}
