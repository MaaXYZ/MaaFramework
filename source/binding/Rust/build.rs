use std::{path::PathBuf, process::Command};

fn cmake_probe(include_dir: &mut Vec<PathBuf>, libs: &mut Vec<PathBuf>) -> Result<(), ()> {
    let out_dir = std::env::var("OUT_DIR").map_err(|_| ())?;

    let cmake_dir = PathBuf::from(out_dir).join("cmake");

    let cmd = Command::new("cmake")
        .arg("./cmake")
        .arg("-B")
        .arg(cmake_dir)
        .arg("-DCMAKE_BUILD_TYPE=Release")
        .output();

    let output = cmd.map_err(|_| ())?;

    let stderr = String::from_utf8(output.stderr).map_err(|_| ())?;

    println!("{}", stderr);
    for line in stderr.lines() {
        if line.starts_with("IncludeDir") {
            let path = line.split('=').nth(1).ok_or(())?;
            include_dir.push(PathBuf::from(path));
        } else if line.starts_with("MaaFrameworkLibraries") {
            let path = line.split('=').nth(1).ok_or(())?;
            libs.push(PathBuf::from(path).parent().unwrap().to_path_buf());
        }
    }

    Ok(())
}

fn main() {
    if std::env::var("DOCS_RS").is_ok() {
        return;
    }

    println!("cargo:rerun-if-changed=./cmake/CMakeLists.txt");
    println!("cargo:rerun-if-changed=headers/wrapper.h");

    let mut include_dir = vec![];
    let mut lib_dir = vec![];

    if cmake_probe(&mut include_dir, &mut lib_dir).is_err() {
        println!("cargo:warning=cmake_probe failed or found no libs, using default paths");
    }

    let manifest_dir = PathBuf::from(std::env::var("CARGO_MANIFEST_DIR").unwrap());
    let local_include = manifest_dir.join("../include");
    if local_include.exists() {
        include_dir.push(local_include);
    }

    // CI/In-tree fallback: check ../install (created by cmake --install)
    let local_install = manifest_dir.join("../install");
    if local_install.exists() {
        let install_lib = local_install.join("lib");
        let install_bin = local_install.join("bin");
        let install_include = local_install.join("include");

        if install_lib.exists() {
            lib_dir.push(install_lib);
        }
        if install_bin.exists() {
            lib_dir.push(install_bin);
        }
        if install_include.exists() && !include_dir.iter().any(|d| d == &install_include) {
            include_dir.push(install_include);
        }
    }

    // Fallback: check for bundled release in the manifest directory (e.g., MAA-win-x86_64-v5.4.1)
    for entry in std::fs::read_dir(&manifest_dir).into_iter().flatten() {
        if let Ok(entry) = entry {
            let path = entry.path();
            if path.is_dir()
                && path
                    .file_name()
                    .map_or(false, |n| n.to_string_lossy().starts_with("MAA-"))
            {
                let bundled_lib = path.join("lib");
                let bundled_bin = path.join("bin");
                let bundled_include = path.join("include");
                if bundled_lib.exists() {
                    lib_dir.push(bundled_lib);
                }
                if bundled_bin.exists() {
                    lib_dir.push(bundled_bin);
                }
                if bundled_include.exists() && !include_dir.iter().any(|d| d == &bundled_include) {
                    include_dir.push(bundled_include);
                }
            }
        }
    }

    for dir in &include_dir {
        println!("cargo:include={}", dir.display());
    }

    for dir in &lib_dir {
        let dir_str = dir.display().to_string();
        if !dir_str.is_empty() {
            println!("cargo:rustc-link-search={}", dir_str);
        }
    }

    println!("cargo:rustc-link-lib=MaaFramework");
    println!("cargo:rustc-link-lib=MaaAgentClient");
    println!("cargo:rustc-link-lib=MaaAgentServer");

    #[cfg(feature = "toolkit")]
    println!("cargo:rustc-link-lib=MaaToolkit");

    let out_path = PathBuf::from(std::env::var("OUT_DIR").unwrap());

    let mut bindings_builder = bindgen::Builder::default()
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .clang_args(include_dir.iter().map(|d| format!("-I{}", d.display())))
        .header("headers/wrapper.h");

    #[cfg(feature = "toolkit")]
    {
        bindings_builder = bindings_builder.header("headers/maa_toolkit.h");
    }

    let bindings = bindings_builder
        .generate()
        .expect("Unable to generate bindings");

    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
