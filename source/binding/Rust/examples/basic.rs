use maa_framework::*;
use serde_json::json;
use std::path::PathBuf;

fn main() -> Result<()> {
    // 设置库路径
    let lib_path = std::env::var("MAA_LIB_PATH").unwrap_or_else(|_| ".".to_string());
    
    #[cfg(target_os = "windows")]
    {
        load_library(format!("{}/MaaFramework.dll", lib_path))?;
        load_toolkit(format!("{}/MaaToolkit.dll", lib_path))?;
    }
    #[cfg(target_os = "linux")]
    {
        load_library(format!("{}/libMaaFramework.so", lib_path))?;
        load_toolkit(format!("{}/libMaaToolkit.so", lib_path))?;
    }
    #[cfg(target_os = "macos")]
    {
        load_library(format!("{}/libMaaFramework.dylib", lib_path))?;
        load_toolkit(format!("{}/libMaaToolkit.dylib", lib_path))?;
    }

    // 打印版本
    println!("MaaFramework 版本: {}", version());

    // 设置日志
    set_log_dir("./log");
    set_stdout_level(LoggingLevel::Info);

    // 查找 ADB 设备
    println!("正在搜索 ADB 设备...");
    let devices = Toolkit::find_adb_devices();

    if devices.is_empty() {
        println!("未找到任何设备");
        return Ok(());
    }

    println!("找到 {} 个设备:", devices.len());
    for (i, device) in devices.iter().enumerate() {
        println!("  [{}] {} - {}", i, device.name, device.address);
    }

    // 使用第一个设备
    let device = &devices[0];
    println!("\n使用设备: {}", device.name);

    // 创建控制器
    let agent_path = std::env::var("MAA_AGENT_PATH").unwrap_or_else(|_| "MaaAgentBinary".to_string());
    let controller = AdbController::new(
        &device.adb_path,
        &device.address,
        adb_screencap_method::DEFAULT,
        adb_input_method::DEFAULT,
        &device.config,
        &agent_path,
    )?;

    // 连接设备
    println!("正在连接设备...");
    let job = controller.post_connection();
    job.wait();

    if !controller.connected() {
        println!("连接失败");
        return Err(MaaError::ConnectionFailed);
    }
    println!("设备已连接, UUID: {}", controller.uuid()?);

    // 截图测试
    println!("\n正在截图...");
    controller.post_screencap().wait();
    let image = controller.cached_image()?;
    println!("截图尺寸: {}x{}", image.width(), image.height());

    // 创建资源
    let resource = Resource::new()?;

    // 加载资源 (如果存在)
    let resource_path = std::env::var("MAA_RESOURCE_PATH").ok().map(PathBuf::from);
    if let Some(path) = resource_path {
        if path.exists() {
            println!("\n正在加载资源: {:?}", path);
            let job = resource.post_bundle(&path);
            job.wait();

            if resource.loaded() {
                println!("资源加载成功");
                println!("资源 Hash: {}", resource.hash()?);

                // 获取节点列表
                let nodes = resource.node_list()?;
                println!("节点数量: {}", nodes.len());
                if !nodes.is_empty() {
                    println!("前 5 个节点:");
                    for node in nodes.iter().take(5) {
                        println!("  - {}", node);
                    }
                }
            } else {
                println!("资源加载失败");
            }
        }
    }

    // 创建任务器
    println!("\n创建任务器...");
    let mut tasker = Tasker::new()?;

    // 需要将 controller 的内部 handle 传递，这里简化处理
    if !tasker.bind_resource(resource) {
        println!("绑定资源失败");
    }

    println!("\n示例完成!");
    Ok(())
}

