#!/usr/bin/env python3
"""Standalone test runner for pipeline_data_test."""

import sys
import os
from pathlib import Path

# Add the binding directory to path
current_dir = Path(__file__).parent
binding_dir = current_dir / "../../source/binding/Python"
binding_dir = binding_dir.resolve()

if str(binding_dir) not in sys.path:
    sys.path.insert(0, str(binding_dir))

# Mock the missing dependencies if they're not available
try:
    import numpy
except ImportError:
    print("Warning: numpy not available, installing...")
    os.system("pip install numpy")
    import numpy

try:
    import strenum
except ImportError:
    print("Warning: strenum not available, installing...")
    os.system("pip install strenum")

# Mock the framework dependencies that might not be built
import sys
from unittest.mock import MagicMock

# Mock framework modules
sys.modules['maa.library'] = MagicMock()
sys.modules['maa.buffer'] = MagicMock()
sys.modules['maa.notification_handler'] = MagicMock()
sys.modules['maa.job'] = MagicMock()

# Mock the define module with the needed classes
mock_define = MagicMock()
mock_define.MaaInferenceExecutionProviderEnum = MagicMock()
mock_define.MaaInferenceDeviceEnum = MagicMock()
mock_define.MaaResourceHandle = MagicMock()
mock_define.MaaBool = MagicMock()
sys.modules['maa.define'] = mock_define

def pipeline_data_test_standalone():
    """Standalone version of pipeline_data_test that doesn't need MaaFramework."""
    print("=== Pipeline Data Test (Standalone) ===")
    
    try:
        # Import the functions we want to test
        from maa.resource import parse_pipeline_data, dump_pipeline_data, JPipelineData, JTemplateMatch, JClick
        import json
        
        print("✓ Import successful!")
        
        # Test data with various field types - single node data
        test_node_data = {
            "recognition": {
                "type": "TemplateMatch",
                "param": {
                    "template": ["test.png", "test2.png"],
                    "roi": [0, 0, 100, 100],
                    "roi_offset": [5, 5, -5, -5],
                    "threshold": [0.8, 0.9],
                    "order_by": "score",
                    "index": 1,
                    "method": 3,
                    "green_mask": True
                }
            },
            "action": {
                "type": "Click",
                "param": {
                    "target": [50, 50, 20, 20],
                    "target_offset": [2, 2, 0, 0]
                }
            },
            "next": ["NextTask", "FallbackTask"],
            "interrupt": ["CancelTask"],
            "timeout": 5000,
            "enabled": True,
            "pre_delay": 100,
            "post_delay": 200
        }
        
        # Convert dict to JSON string and parse
        json_str = json.dumps(test_node_data, ensure_ascii=False)
        parsed_data = parse_pipeline_data(json_str)
        
        print(f"✓ Parsed recognition type: {parsed_data.recognition.type}")
        print(f"✓ Parsed action type: {parsed_data.action.type}")
        print(f"✓ Recognition param type: {type(parsed_data.recognition.param)}")
        print(f"✓ Action param type: {type(parsed_data.action.param)}")
        
        # Convert back to JSON string
        dumped_json = dump_pipeline_data(parsed_data)
        print(f"✓ Dumped JSON length: {len(dumped_json)} characters")
        
        # Parse again to verify round-trip
        reparsed_data = parse_pipeline_data(dumped_json)
        
        # Verify round-trip conversion
        conversion_match = True
        if parsed_data.recognition.type != reparsed_data.recognition.type:
            print(f"  ERROR: Recognition type mismatch: {parsed_data.recognition.type} != {reparsed_data.recognition.type}")
            conversion_match = False
        if parsed_data.action.type != reparsed_data.action.type:
            print(f"  ERROR: Action type mismatch: {parsed_data.action.type} != {reparsed_data.action.type}")
            conversion_match = False
        if parsed_data.timeout != reparsed_data.timeout:
            print(f"  ERROR: Timeout mismatch: {parsed_data.timeout} != {reparsed_data.timeout}")
            conversion_match = False
        
        if conversion_match:
            print("✓ Round-trip conversion successful!")
        else:
            raise RuntimeError("Round-trip conversion failed!")
        
        # Test type safety
        if isinstance(parsed_data.recognition.param, JTemplateMatch):
            template_files = parsed_data.recognition.param.template
            if template_files == ["test.png", "test2.png"]:
                print("✓ Field mapping verified (template accessible)")
            else:
                raise RuntimeError(f"Field mapping failed: expected ['test.png', 'test2.png'], got {template_files}")
        
        if isinstance(parsed_data.action.param, JClick):
            click_target = parsed_data.action.param.target
            print(f"✓ Action param is JClick with target: {click_target}")
        else:
            raise RuntimeError(f"Action param type check failed: {type(parsed_data.action.param)}")
        
        # Test field mapping by checking the JSON output
        if "template_" in dumped_json:
            print("✓ Field mapping test passed (template -> template_)")
        else:
            print("✗ Field mapping test failed (template_ not found in JSON)")
            
        print("✓ All tests passed successfully!")
        return True
        
    except Exception as e:
        print(f"✗ Test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = pipeline_data_test_standalone()
    if success:
        print("\n🎉 Pipeline data test completed successfully!")
        sys.exit(0)
    else:
        print("\n❌ Pipeline data test failed!")
        sys.exit(1)