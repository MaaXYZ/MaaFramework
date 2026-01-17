//! Comprehensive Pipeline Type System Tests
//!
//! Port of Python's pipeline_test.py - following "brute force" testing principle.
//! Tests MUST discover real errors. We do NOT bypass failures.
//!
//! Test coverage:
//! 1. Recognition types: DirectHit, TemplateMatch, FeatureMatch, ColorMatch, OCR,
//!    NeuralNetworkClassify, NeuralNetworkDetect, And, Or, Custom
//! 2. Action types: Click, LongPress, Swipe, MultiSwipe, InputText, StartApp,
//!    StopApp, Command, Shell, Custom, DoNothing
//! 3. Node attributes: next, on_error, JumpBack, Anchor, object format
//! 4. Override inheritance: And/Or override behavior
//! 5. v2 format: {"type": "...", "param": {...}} style
//! 6. wait_freezes and repeat parameters

mod common;

use std::sync::atomic::{AtomicBool, Ordering};

use maa_framework::context::Context;
use maa_framework::controller::Controller;
use maa_framework::custom::{CustomAction, CustomRecognition};
use maa_framework::pipeline::{ActionType, RecognitionType};
use maa_framework::resource::Resource;
use maa_framework::tasker::Tasker;
use maa_framework::{self, sys, MaaResult};

use common::{get_test_resources_dir, init_test_env, ImageController};

// ============================================================================
// Recognition Type Tests - Ported from Python _test_recognition_types
// ============================================================================

static CONTEXT_TESTS_PASSED: AtomicBool = AtomicBool::new(false);

/// Custom recognition that runs all Context-level pipeline tests
struct PipelineTestRecognition;

impl CustomRecognition for PipelineTestRecognition {
    fn analyze(
        &self,
        context: &Context,
        _task_id: sys::MaaTaskId,
        _node_name: &str,
        _custom_recognition_name: &str,
        _custom_recognition_param: &str,
        _image: *const sys::MaaImageBuffer,
        _roi: &sys::MaaRect,
    ) -> Option<(sys::MaaRect, String)> {
        println!("\n=== PipelineTestRecognition.analyze ===");

        // Run all Context-level tests
        let result = run_context_pipeline_tests(context);

        if result.is_ok() {
            CONTEXT_TESTS_PASSED.store(true, Ordering::SeqCst);
        } else {
            println!("  Context tests FAILED: {:?}", result);
        }

        Some((
            sys::MaaRect {
                x: 0,
                y: 0,
                width: 100,
                height: 100,
            },
            r#"{"test": "passed"}"#.to_string(),
        ))
    }
}

struct PipelineTestAction;

impl CustomAction for PipelineTestAction {
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
        true
    }
}

/// Run all Context-level pipeline tests - matching Python's _run_context_tests
fn run_context_pipeline_tests(context: &Context) -> MaaResult<()> {
    test_context_get_node_data(context).map_err(|e| {
        println!("FAILED: test_context_get_node_data: {:?}", e);
        e
    })?;
    test_context_get_node_object(context).map_err(|e| {
        println!("FAILED: test_context_get_node_object: {:?}", e);
        e
    })?;
    test_context_override_pipeline(context).map_err(|e| {
        println!("FAILED: test_context_override_pipeline: {:?}", e);
        e
    })?;
    test_context_override_next(context).map_err(|e| {
        println!("FAILED: test_context_override_next: {:?}", e);
        e
    })?;

    test_and_or_override_inheritance(context).map_err(|e| {
        println!("FAILED: test_and_or_override_inheritance: {:?}", e);
        e
    })?;
    test_recognition_types(context).map_err(|e| {
        println!("FAILED: test_recognition_types: {:?}", e);
        e
    })?;
    test_action_types(context).map_err(|e| {
        println!("FAILED: test_action_types: {:?}", e);
        e
    })?;
    test_node_attributes(context).map_err(|e| {
        println!("FAILED: test_node_attributes: {:?}", e);
        e
    })?;
    test_v2_format(context).map_err(|e| {
        println!("FAILED: test_v2_format: {:?}", e);
        e
    })?;
    test_wait_freezes(context).map_err(|e| {
        println!("FAILED: test_wait_freezes: {:?}", e);
        e
    })?;
    test_repeat_params(context).map_err(|e| {
        println!("FAILED: test_repeat_params: {:?}", e);
        e
    })?;

    println!("  All Context-level pipeline tests PASSED");
    Ok(())
}

/// Test context.get_node_data - returns raw JSON string
fn test_context_get_node_data(context: &Context) -> MaaResult<()> {
    println!("  Testing context.get_node_data...");

    let new_ctx = context.clone_context()?;
    new_ctx.override_pipeline(r#"{"TestDataNode": {"action": "DoNothing"}}"#)?;

    let node_data = new_ctx.get_node_data("TestDataNode")?;
    assert!(node_data.is_some(), "get_node_data should return Some");

    let data = node_data.unwrap();
    assert!(data.contains("action"), "node_data MUST contain 'action'");

    println!("    PASS: context.get_node_data");
    Ok(())
}

/// Test context.get_node_object - returns parsed PipelineData
fn test_context_get_node_object(context: &Context) -> MaaResult<()> {
    println!("  Testing context.get_node_object...");

    let new_ctx = context.clone_context()?;
    new_ctx.override_pipeline(r#"{"TestObjectNode": {}}"#)?;

    let node_obj = new_ctx.get_node_object("TestObjectNode")?;
    assert!(node_obj.is_some(), "get_node_object should return Some");

    let obj = node_obj.unwrap();
    // Default recognition is DirectHit
    assert_eq!(obj.recognition.recognition_type, RecognitionType::DirectHit);
    // Default action is DoNothing
    assert_eq!(obj.action.action_type, ActionType::DoNothing);

    println!("    PASS: context.get_node_object");
    Ok(())
}

/// Test context.override_pipeline - matching Python exactly
fn test_context_override_pipeline(context: &Context) -> MaaResult<()> {
    println!("  Testing context.override_pipeline...");

    let new_ctx = context.clone_context()?;

    // First create referenced nodes (framework checks next list)
    new_ctx.override_pipeline(r#"{"NodeA": {}, "NodeB": {}}"#)?;

    // Create comprehensive test node
    let override_json = r#"{
        "OverrideTestNode": {
            "recognition": "OCR",
            "expected": ["TestText"],
            "action": "Click",
            "target": [100, 200, 50, 50],
            "next": ["NodeA", "NodeB"],
            "timeout": 5000,
            "rate_limit": 500,
            "pre_delay": 100,
            "post_delay": 300,
            "max_hit": 3,
            "enabled": true,
            "inverse": false,
            "anchor": ["my_anchor"],
            "focus": {"key": "value"},
            "attach": {"custom_data": 123}
        }
    }"#;
    new_ctx.override_pipeline(override_json)?;

    let node_obj = new_ctx
        .get_node_object("OverrideTestNode")?
        .expect("OverrideTestNode MUST exist");

    // Verify recognition
    assert_eq!(
        node_obj.recognition.recognition_type,
        RecognitionType::OCR,
        "recognition type MUST be OCR"
    );

    // Verify action
    assert_eq!(
        node_obj.action.action_type,
        ActionType::Click,
        "action type MUST be Click"
    );

    // Verify other fields
    assert_eq!(node_obj.timeout, 5000, "timeout");
    assert_eq!(node_obj.rate_limit, 500, "rate_limit");
    assert_eq!(node_obj.pre_delay, 100, "pre_delay");
    assert_eq!(node_obj.post_delay, 300, "post_delay");
    assert_eq!(node_obj.max_hit, 3, "max_hit");
    assert_eq!(node_obj.enabled, true, "enabled");
    assert_eq!(node_obj.inverse, false, "inverse");
    assert_eq!(node_obj.anchor, vec!["my_anchor".to_string()], "anchor");

    // Verify attach
    assert!(node_obj.attach.is_some(), "attach should exist");
    let attach = node_obj.attach.as_ref().unwrap();
    assert_eq!(
        attach.get("custom_data").and_then(|v| v.as_i64()),
        Some(123)
    );

    // Verify next list
    assert_eq!(node_obj.next.len(), 2, "next length");
    assert_eq!(node_obj.next[0].name, "NodeA", "next[0].name");
    assert_eq!(node_obj.next[1].name, "NodeB", "next[1].name");

    println!("    PASS: context.override_pipeline");
    Ok(())
}

/// Test context.override_next - matching Python exactly
fn test_context_override_next(context: &Context) -> MaaResult<()> {
    println!("  Testing context.override_next...");

    let new_ctx = context.clone_context()?;

    // Create referenced nodes first
    new_ctx.override_pipeline(
        r#"{
        "OverrideNextTestNode": {},
        "NextNode1": {},
        "NextNode2": {},
        "MyAnchor": {"anchor": ["MyAnchor"]}
    }"#,
    )?;

    // Use override_next to modify next list
    let result = new_ctx.override_next(
        "OverrideNextTestNode",
        &["NextNode1", "[JumpBack]NextNode2", "[Anchor]MyAnchor"],
    )?;
    assert!(result, "override_next should succeed");

    let node_obj = new_ctx
        .get_node_object("OverrideNextTestNode")?
        .expect("Node should exist");

    assert_eq!(node_obj.next.len(), 3, "next length after override");
    assert_eq!(node_obj.next[0].name, "NextNode1");
    assert_eq!(node_obj.next[0].jump_back, false);
    assert_eq!(node_obj.next[1].name, "NextNode2");
    assert_eq!(node_obj.next[1].jump_back, true);
    assert_eq!(node_obj.next[2].name, "MyAnchor");
    assert_eq!(node_obj.next[2].anchor, true);

    println!("    PASS: context.override_next");
    Ok(())
}

/// Test And/Or override inheritance - matching Python exactly
fn test_and_or_override_inheritance(context: &Context) -> MaaResult<()> {
    println!("  Testing And/Or override inheritance...");

    let new_ctx = context.clone_context()?;

    // Create And recognition node
    new_ctx.override_pipeline(
        r#"{
        "AndTestNode": {
            "recognition": {
                "type": "And",
                "param": {
                    "all_of": [
                        {"recognition": {"type": "DirectHit"}},
                        {"recognition": {"type": "DirectHit"}}
                    ],
                    "box_index": 1
                }
            }
        }
    }"#,
    )?;

    // Override only box_index, all_of should be inherited
    new_ctx.override_pipeline(
        r#"{
        "AndTestNode": {
            "recognition": {"param": {"box_index": 0}}
        }
    }"#,
    )?;

    let and_node = new_ctx
        .get_node_object("AndTestNode")?
        .expect("AndTestNode MUST exist");

    assert_eq!(
        and_node.recognition.recognition_type,
        RecognitionType::And,
        "And type preserved"
    );

    // Verify all_of was inherited
    let all_of = and_node
        .recognition
        .param
        .all_of
        .as_ref()
        .expect("all_of should be inherited");
    assert_eq!(all_of.len(), 2, "all_of should have 2 elements");

    // Verify box_index was updated
    assert_eq!(
        and_node.recognition.param.box_index,
        Some(0),
        "box_index should be updated to 0"
    );

    // Test Or recognition
    new_ctx.override_pipeline(
        r#"{
        "OrTestNode": {
            "recognition": {
                "type": "Or",
                "param": {
                    "any_of": [
                        {"recognition": {"type": "DirectHit"}}
                    ]
                }
            }
        }
    }"#,
    )?;

    // Override with empty object, any_of should be inherited
    new_ctx.override_pipeline(r#"{"OrTestNode": {}}"#)?;

    let or_node = new_ctx
        .get_node_object("OrTestNode")?
        .expect("OrTestNode MUST exist");

    assert_eq!(
        or_node.recognition.recognition_type,
        RecognitionType::Or,
        "Or type preserved"
    );

    let any_of = or_node
        .recognition
        .param
        .any_of
        .as_ref()
        .expect("any_of should be inherited");
    assert_eq!(any_of.len(), 1, "any_of should have 1 element");

    println!("    PASS: And/Or override inheritance");
    Ok(())
}

/// Test all recognition types - matching Python _test_recognition_types
fn test_recognition_types(context: &Context) -> MaaResult<()> {
    println!("  Testing recognition types parsing...");

    let new_ctx = context.clone_context()?;

    // TemplateMatch
    new_ctx.override_pipeline(
        r#"{
        "RecoTemplateMatch": {
            "recognition": "TemplateMatch",
            "template": ["test.png"],
            "threshold": 0.8,
            "roi": [10, 20, 100, 200],
            "order_by": "Score",
            "index": 1,
            "method": 3,
            "green_mask": true
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("RecoTemplateMatch")?.expect("node");
    assert_eq!(
        obj.recognition.recognition_type,
        RecognitionType::TemplateMatch
    );
    assert_eq!(
        obj.recognition.param.template,
        Some(vec!["test.png".to_string()])
    );
    assert_eq!(obj.recognition.param.threshold, Some(vec![0.8]));
    assert_eq!(obj.recognition.param.order_by, Some("Score".to_string()));
    assert_eq!(obj.recognition.param.index, Some(1));
    assert_eq!(obj.recognition.param.method, Some(3));
    assert_eq!(obj.recognition.param.green_mask, Some(true));

    // FeatureMatch
    new_ctx.override_pipeline(
        r#"{
        "RecoFeatureMatch": {
            "recognition": "FeatureMatch",
            "template": ["feature.png"],
            "detector": "ORB",
            "count": 10,
            "ratio": 0.75
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("RecoFeatureMatch")?.expect("node");
    assert_eq!(
        obj.recognition.recognition_type,
        RecognitionType::FeatureMatch
    );
    assert_eq!(obj.recognition.param.detector, Some("ORB".to_string()));
    assert_eq!(obj.recognition.param.count, Some(10));
    assert_eq!(obj.recognition.param.ratio, Some(0.75));

    // ColorMatch
    new_ctx.override_pipeline(
        r#"{
        "RecoColorMatch": {
            "recognition": "ColorMatch",
            "lower": [[100, 100, 100]],
            "upper": [[255, 255, 255]],
            "count": 50,
            "method": 40,
            "connected": true
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("RecoColorMatch")?.expect("node");
    assert_eq!(
        obj.recognition.recognition_type,
        RecognitionType::ColorMatch
    );
    assert_eq!(obj.recognition.param.lower, Some(vec![vec![100, 100, 100]]));
    assert_eq!(obj.recognition.param.upper, Some(vec![vec![255, 255, 255]]));
    assert_eq!(obj.recognition.param.count, Some(50));
    assert_eq!(obj.recognition.param.method, Some(40));
    assert_eq!(obj.recognition.param.connected, Some(true));

    // OCR
    new_ctx.override_pipeline(
        r#"{
        "RecoOCR": {
            "recognition": "OCR",
            "expected": ["Hello", "World"],
            "threshold": 0.5,
            "replace": [["0", "O"], ["1", "I"]],
            "only_rec": true,
            "model": "custom_model"
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("RecoOCR")?.expect("node");
    assert_eq!(obj.recognition.recognition_type, RecognitionType::OCR);
    assert!(
        obj.recognition
            .param
            .expected
            .as_ref()
            .map_or(false, |v| v.len() == 2),
        "expected should have 2 elements"
    );
    assert_eq!(obj.recognition.param.only_rec, Some(true));
    assert_eq!(
        obj.recognition.param.model,
        Some("custom_model".to_string())
    );

    // NeuralNetworkClassify
    new_ctx.override_pipeline(
        r#"{
        "RecoNNClassify": {
            "recognition": "NeuralNetworkClassify",
            "model": "classify.onnx",
            "expected": [0, 2],
            "labels": ["Cat", "Dog", "Mouse"]
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("RecoNNClassify")?.expect("node");
    assert_eq!(
        obj.recognition.recognition_type,
        RecognitionType::NeuralNetworkClassify
    );
    assert_eq!(
        obj.recognition.param.model,
        Some("classify.onnx".to_string())
    );
    assert_eq!(
        obj.recognition.param.labels,
        Some(vec![
            "Cat".to_string(),
            "Dog".to_string(),
            "Mouse".to_string()
        ])
    );

    // NeuralNetworkDetect
    new_ctx.override_pipeline(
        r#"{
        "RecoNNDetect": {
            "recognition": "NeuralNetworkDetect",
            "model": "detect.onnx",
            "expected": [1],
            "threshold": [0.5]
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("RecoNNDetect")?.expect("node");
    assert_eq!(
        obj.recognition.recognition_type,
        RecognitionType::NeuralNetworkDetect
    );
    assert_eq!(obj.recognition.param.model, Some("detect.onnx".to_string()));
    assert_eq!(obj.recognition.param.threshold, Some(vec![0.5]));

    // Custom
    new_ctx.override_pipeline(
        r#"{
        "RecoCustom": {
            "recognition": "Custom",
            "custom_recognition": "MyCustomReco",
            "custom_recognition_param": {"key": "value"},
            "roi": [0, 0, 100, 100]
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("RecoCustom")?.expect("node");
    assert_eq!(obj.recognition.recognition_type, RecognitionType::Custom);
    assert_eq!(
        obj.recognition.param.custom_recognition,
        Some("MyCustomReco".to_string())
    );
    let custom_param = obj
        .recognition
        .param
        .custom_recognition_param
        .as_ref()
        .expect("param");
    assert_eq!(
        custom_param.get("key").and_then(|v| v.as_str()),
        Some("value")
    );

    println!("    PASS: recognition types parsing");
    Ok(())
}

/// Test all action types - matching Python _test_action_types
fn test_action_types(context: &Context) -> MaaResult<()> {
    println!("  Testing action types parsing...");

    let new_ctx = context.clone_context()?;

    // Click
    new_ctx.override_pipeline(
        r#"{
        "ActClick": {
            "action": "Click",
            "target": [100, 200, 50, 50],
            "target_offset": [10, 10, 0, 0],
            "contact": 1
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("ActClick")?.expect("node");
    assert_eq!(obj.action.action_type, ActionType::Click);
    assert_eq!(obj.action.param.contact, Some(1));

    // LongPress
    new_ctx.override_pipeline(
        r#"{
        "ActLongPress": {
            "action": "LongPress",
            "duration": 2000
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("ActLongPress")?.expect("node");
    assert_eq!(obj.action.action_type, ActionType::LongPress);
    assert_eq!(obj.action.param.duration, Some(vec![2000]));

    // Swipe - test with [x, y] point format
    new_ctx.override_pipeline(
        r#"{
        "ActSwipe": {
            "action": "Swipe",
            "begin": [100, 100],
            "end": [300, 300],
            "duration": 500
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("ActSwipe")?.expect("node");
    assert_eq!(obj.action.action_type, ActionType::Swipe);

    // MultiSwipe - test with swipes array
    new_ctx.override_pipeline(
        r#"{
        "ActMultiSwipe": {
            "action": "MultiSwipe",
            "swipes": [
                {"begin": [100, 100], "end": [200, 200]},
                {"starting": 500, "begin": [300, 300], "end": [400, 400]}
            ]
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("ActMultiSwipe")?.expect("node");
    assert_eq!(obj.action.action_type, ActionType::MultiSwipe);
    let swipes = obj.action.param.swipes.as_ref().expect("swipes");
    assert_eq!(swipes.len(), 2);

    // InputText
    new_ctx.override_pipeline(
        r#"{
        "ActInputText": {
            "action": "InputText",
            "input_text": "Hello World"
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("ActInputText")?.expect("node");
    assert_eq!(obj.action.action_type, ActionType::InputText);
    assert_eq!(obj.action.param.input_text, Some("Hello World".to_string()));

    // StartApp
    new_ctx.override_pipeline(
        r#"{
        "ActStartApp": {
            "action": "StartApp",
            "package": "com.example.app"
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("ActStartApp")?.expect("node");
    assert_eq!(obj.action.action_type, ActionType::StartApp);
    assert_eq!(
        obj.action.param.package,
        Some("com.example.app".to_string())
    );

    // StopApp
    new_ctx.override_pipeline(
        r#"{
        "ActStopApp": {
            "action": "StopApp",
            "package": "com.example.app"
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("ActStopApp")?.expect("node");
    assert_eq!(obj.action.action_type, ActionType::StopApp);
    assert_eq!(
        obj.action.param.package,
        Some("com.example.app".to_string())
    );

    // Command
    new_ctx.override_pipeline(
        r#"{
        "ActCommand": {
            "action": "Command",
            "exec": "python",
            "args": ["script.py", "--arg1"],
            "detach": true
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("ActCommand")?.expect("node");
    assert_eq!(obj.action.action_type, ActionType::Command);
    assert_eq!(obj.action.param.exec, Some("python".to_string()));
    assert_eq!(
        obj.action.param.args,
        Some(vec!["script.py".to_string(), "--arg1".to_string()])
    );
    assert_eq!(obj.action.param.detach, Some(true));

    // Shell
    new_ctx.override_pipeline(
        r#"{
        "ActShell": {
            "action": "Shell",
            "cmd": "ls -la",
            "timeout": 30000
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("ActShell")?.expect("node");
    assert_eq!(obj.action.action_type, ActionType::Shell);
    assert_eq!(obj.action.param.cmd, Some("ls -la".to_string()));
    assert_eq!(obj.action.param.timeout, Some(30000));

    // Custom
    new_ctx.override_pipeline(
        r#"{
        "ActCustom": {
            "action": "Custom",
            "custom_action": "MyCustomAction",
            "custom_action_param": {"data": 123}
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("ActCustom")?.expect("node");
    assert_eq!(obj.action.action_type, ActionType::Custom);
    assert_eq!(
        obj.action.param.custom_action,
        Some("MyCustomAction".to_string())
    );
    let custom_param = obj
        .action
        .param
        .custom_action_param
        .as_ref()
        .expect("param");
    assert_eq!(custom_param.get("data").and_then(|v| v.as_i64()), Some(123));

    // DoNothing
    new_ctx.override_pipeline(
        r#"{
        "ActDoNothing": {
            "action": "DoNothing"
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("ActDoNothing")?.expect("node");
    assert_eq!(obj.action.action_type, ActionType::DoNothing);

    println!("    PASS: action types parsing");
    Ok(())
}

/// Test node attributes - matching Python _test_node_attributes
fn test_node_attributes(context: &Context) -> MaaResult<()> {
    println!("  Testing node attributes...");

    let new_ctx = context.clone_context()?;

    // Create referenced nodes first
    new_ctx.override_pipeline(
        r#"{
        "PlainNode": {},
        "JumpBackNode": {},
        "AnchorRef": {"anchor": ["AnchorRef"]},
        "ObjectNode": {},
        "AnchorObjNode": {"anchor": ["AnchorObjNode"]},
        "ErrorHandler": {}
    }"#,
    )?;

    // Test next list with various node attributes
    new_ctx.override_pipeline(
        r#"{
        "NodeAttrTest": {
            "next": [
                "PlainNode",
                "[JumpBack]JumpBackNode",
                "[Anchor]AnchorRef",
                {"name": "ObjectNode", "jump_back": true},
                {"name": "AnchorObjNode", "anchor": true}
            ],
            "on_error": ["[JumpBack]ErrorHandler"]
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("NodeAttrTest")?.expect("node");

    // Verify next
    assert_eq!(obj.next.len(), 5, "next length");
    assert_eq!(obj.next[0].name, "PlainNode");
    assert_eq!(obj.next[0].jump_back, false);
    assert_eq!(obj.next[0].anchor, false);

    assert_eq!(obj.next[1].name, "JumpBackNode");
    assert_eq!(obj.next[1].jump_back, true);

    assert_eq!(obj.next[2].name, "AnchorRef");
    assert_eq!(obj.next[2].anchor, true);

    assert_eq!(obj.next[3].name, "ObjectNode");
    assert_eq!(obj.next[3].jump_back, true);

    assert_eq!(obj.next[4].name, "AnchorObjNode");
    assert_eq!(obj.next[4].anchor, true);

    // Verify on_error
    assert_eq!(obj.on_error.len(), 1, "on_error length");
    assert_eq!(obj.on_error[0].name, "ErrorHandler");
    assert_eq!(obj.on_error[0].jump_back, true);

    println!("    PASS: node attributes");
    Ok(())
}

/// Test v2 format parsing - matching Python _test_v2_format
fn test_v2_format(context: &Context) -> MaaResult<()> {
    println!("  Testing v2 format parsing...");

    let new_ctx = context.clone_context()?;

    // v2 format with explicit type and param
    new_ctx.override_pipeline(
        r#"{
        "V2FormatNode": {
            "recognition": {
                "type": "TemplateMatch",
                "param": {
                    "template": ["v2.png"],
                    "threshold": 0.9,
                    "roi": [0, 0, 100, 100]
                }
            },
            "action": {
                "type": "Click",
                "param": {"target": true, "contact": 2}
            },
            "pre_delay": 50,
            "post_delay": 150
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("V2FormatNode")?.expect("node");

    assert_eq!(
        obj.recognition.recognition_type,
        RecognitionType::TemplateMatch
    );
    assert_eq!(
        obj.recognition.param.template,
        Some(vec!["v2.png".to_string()])
    );
    assert_eq!(obj.recognition.param.threshold, Some(vec![0.9]));

    assert_eq!(obj.action.action_type, ActionType::Click);
    assert_eq!(obj.action.param.contact, Some(2));

    assert_eq!(obj.pre_delay, 50);
    assert_eq!(obj.post_delay, 150);

    println!("    PASS: v2 format parsing");
    Ok(())
}

/// Test wait_freezes parameter parsing
fn test_wait_freezes(context: &Context) -> MaaResult<()> {
    println!("  Testing wait_freezes...");

    let new_ctx = context.clone_context()?;

    new_ctx.override_pipeline(
        r#"{
        "WaitFreezesTest": {
            "pre_wait_freezes": {
                "time": 500,
                "threshold": 0.99,
                "method": 3
            },
            "post_wait_freezes": {
                "time": 1000,
                "target": [10, 20, 30, 40]
            }
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("WaitFreezesTest")?.expect("node");

    let pre = obj.pre_wait_freezes.as_ref().expect("pre_wait_freezes");
    assert_eq!(pre.time, 500);
    assert_eq!(pre.threshold, 0.99);
    assert_eq!(pre.method, 3);

    let post = obj.post_wait_freezes.as_ref().expect("post_wait_freezes");
    assert_eq!(post.time, 1000);

    println!("    PASS: wait_freezes");
    Ok(())
}

/// Test repeat parameter parsing
fn test_repeat_params(context: &Context) -> MaaResult<()> {
    println!("  Testing repeat params...");

    let new_ctx = context.clone_context()?;

    new_ctx.override_pipeline(
        r#"{
        "RepeatTest": {
            "repeat": 5,
            "repeat_delay": 300,
            "repeat_wait_freezes": {"time": 200, "threshold": 0.9}
        }
    }"#,
    )?;

    let obj = new_ctx.get_node_object("RepeatTest")?.expect("node");

    assert_eq!(obj.repeat, 5);
    assert_eq!(obj.repeat_delay, 300);

    let repeat_freezes = obj
        .repeat_wait_freezes
        .as_ref()
        .expect("repeat_wait_freezes");
    assert_eq!(repeat_freezes.time, 200);
    assert_eq!(repeat_freezes.threshold, 0.9);

    println!("    PASS: repeat params");
    Ok(())
}

// ============================================================================
// Main Test Entry Points
// ============================================================================

#[test]
fn test_pipeline_full_execution() {
    println!("\n=== test_pipeline_full_execution ===");
    init_test_env().unwrap();

    let res_dir = get_test_resources_dir();
    let resource_dir = res_dir.join("resource");
    let screenshot_dir = res_dir.join("Screenshot");

    assert!(
        resource_dir.exists(),
        "Test resources MUST exist at {:?}. Run: git submodule update --init",
        resource_dir
    );
    assert!(
        screenshot_dir.exists(),
        "Screenshot directory MUST exist at {:?}",
        screenshot_dir
    );

    // 1. Create Resource
    let resource = Resource::new().unwrap();
    let id = resource
        .post_bundle(resource_dir.to_str().unwrap())
        .unwrap();
    resource.wait(id);
    assert!(resource.loaded(), "Resource MUST be loaded");

    // 2. Register custom recognition for Context-level tests
    resource
        .register_custom_recognition("PipelineTestReco", Box::new(PipelineTestRecognition))
        .unwrap();
    resource
        .register_custom_action("PipelineTestAct", Box::new(PipelineTestAction))
        .unwrap();

    // 3. Create Controller (matching Python's DbgController with CarouselImage)
    let img_ctrl = ImageController::new(screenshot_dir);
    let controller = Controller::new_custom(img_ctrl).unwrap();
    let conn_id = controller.post_connection().unwrap();
    controller.wait(conn_id);
    assert!(controller.connected(), "Controller MUST be connected");

    // 4. Create Tasker
    let tasker = Tasker::new().unwrap();
    tasker.bind_resource(&resource).unwrap();
    tasker.bind_controller(&controller).unwrap();

    assert!(tasker.inited(), "Tasker MUST be initialized");

    // 5. Run task to trigger Context-level tests
    let ppover = r#"{
        "TestEntry": {"next": ["TestReco"]},
        "TestReco": {
            "recognition": "Custom",
            "custom_recognition": "PipelineTestReco",
            "action": "Custom",
            "custom_action": "PipelineTestAct"
        }
    }"#;

    let job = tasker
        .post_task("TestEntry", ppover)
        .expect("post_task MUST work");
    let status = job.wait();

    assert!(status.done(), "Task MUST complete");

    // 6. Verify Context tests passed
    assert!(
        CONTEXT_TESTS_PASSED.load(Ordering::SeqCst),
        "Context-level pipeline tests MUST pass"
    );

    println!("PASS: pipeline full execution");
}

#[test]
fn test_resource_get_node_data() {
    println!("\n=== test_resource_get_node_data ===");
    init_test_env().unwrap();

    let res_dir = get_test_resources_dir();
    let resource_dir = res_dir.join("resource");

    assert!(
        resource_dir.exists(),
        "Test resources not found at {:?}",
        resource_dir
    );

    let resource = Resource::new().unwrap();
    let id = resource
        .post_bundle(resource_dir.to_str().unwrap())
        .unwrap();
    resource.wait(id);

    assert!(resource.loaded(), "Resource MUST be loaded");

    // Test node_list
    let node_list = resource.node_list().expect("node_list MUST work");
    println!("  node_list count: {}", node_list.len());
    assert!(!node_list.is_empty(), "node_list MUST NOT be empty");

    // Test get_node_data for existing node
    let first_node = &node_list[0];
    let node_data = resource
        .get_node_data(first_node)
        .expect("get_node_data MUST NOT error");
    assert!(
        node_data.is_some(),
        "get_node_data MUST return Some for existing node"
    );

    let data = node_data.unwrap();
    // Debug: write JSON to file for clean analysis
    let debug_path = std::env::temp_dir().join("maa_node_json.txt");
    std::fs::write(&debug_path, &data).expect("write file");
    println!("  JSON written to: {:?}", debug_path);

    // Verify it's valid JSON
    let parsed: Result<serde_json::Value, _> = serde_json::from_str(&data);
    assert!(parsed.is_ok(), "node_data MUST be valid JSON");

    // Test get_node_data for non-existent node
    let non_exist = resource
        .get_node_data("NonExistentNode12345")
        .expect("get_node_data should not error");
    assert!(non_exist.is_none(), "Non-existent node should return None");

    println!("PASS: resource_get_node_data");
}

#[test]
fn test_resource_get_node_object() {
    println!("\n=== test_resource_get_node_object ===");
    init_test_env().unwrap();

    let res_dir = get_test_resources_dir();
    let resource_dir = res_dir.join("resource");

    assert!(
        resource_dir.exists(),
        "Test resources MUST exist at {:?}",
        resource_dir
    );

    let resource = Resource::new().unwrap();
    let id = resource
        .post_bundle(resource_dir.to_str().unwrap())
        .unwrap();
    resource.wait(id);

    assert!(resource.loaded(), "Resource MUST load successfully");

    let node_list = resource.node_list().expect("node_list MUST work");
    assert!(!node_list.is_empty(), "Node list MUST NOT be empty");

    // Test get_node_object - MUST parse successfully
    let first_node = &node_list[0];
    let obj = resource
        .get_node_object(first_node)
        .expect("get_node_object MUST NOT error")
        .expect("get_node_object MUST return Some");

    println!("  Testing node: {}", first_node);
    println!(
        "    recognition.type: {:?}",
        obj.recognition.recognition_type
    );
    println!("    action.type: {:?}", obj.action.action_type);
    println!("    next count: {}", obj.next.len());

    println!("PASS: resource_get_node_object (STRICT)");
}

#[test]
fn test_resource_override_pipeline() {
    println!("\n=== test_resource_override_pipeline ===");
    init_test_env().unwrap();

    let res_dir = get_test_resources_dir();
    let resource_dir = res_dir.join("resource");

    assert!(resource_dir.exists(), "Test resources MUST exist");

    let resource = Resource::new().unwrap();
    let id = resource
        .post_bundle(resource_dir.to_str().unwrap())
        .unwrap();
    resource.wait(id);

    assert!(resource.loaded(), "Resource MUST load successfully");

    // Test override_pipeline
    let override_json = r#"{
        "TestOverrideNode": {
            "recognition": "DirectHit",
            "action": "Click",
            "target": [100, 200, 50, 50]
        }
    }"#;

    resource
        .override_pipeline(override_json)
        .expect("override_pipeline MUST succeed");
    println!("  override_pipeline: OK");

    // Verify the new node exists
    let node_data = resource
        .get_node_data("TestOverrideNode")
        .expect("get_node_data MUST NOT error")
        .expect("TestOverrideNode MUST exist after override");

    assert!(
        node_data.contains("DirectHit"),
        "Override node MUST have DirectHit recognition"
    );
    assert!(
        node_data.contains("Click"),
        "Override node MUST have Click action"
    );

    println!("PASS: resource_override_pipeline (STRICT)");
}

#[test]
fn test_resource_hash() {
    println!("\n=== test_resource_hash ===");
    init_test_env().unwrap();

    let res_dir = get_test_resources_dir();
    let resource_dir = res_dir.join("resource");

    assert!(resource_dir.exists(), "Test resources MUST exist");

    let resource = Resource::new().unwrap();
    let id = resource
        .post_bundle(resource_dir.to_str().unwrap())
        .unwrap();
    resource.wait(id);

    assert!(resource.loaded(), "Resource MUST load successfully");

    // Test hash property
    let hash = resource.hash().expect("hash MUST NOT error");
    assert!(!hash.is_empty(), "Hash MUST NOT be empty");
    assert!(
        hash.len() >= 8,
        "Hash MUST be at least 8 chars, got {}",
        hash.len()
    );
    println!("  resource.hash: {}", hash);

    println!("PASS: resource_hash (STRICT)");
}
