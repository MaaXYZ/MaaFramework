"""Deterministic API contracts executed inside the real AgentServer process."""

import numpy

from maa.pipeline import (
    JActionType,
    JClick,
    JColorMatch,
    JDirectHit,
    JDoNothing,
    JRecognitionType,
)


def context_clone_isolation(context):
    name = "AgentContractLocal"
    assert context.get_node_data(name) is None
    clone = context.clone()
    assert clone.override_pipeline({name: {"pre_delay": 7}})
    assert clone.get_node_data(name)["pre_delay"] == 7
    assert context.get_node_data(name) is None
    assert context.tasker.resource.get_node_data(name) is None

    second_clone = clone.clone()
    assert second_clone.override_pipeline({name: {"pre_delay": 11}})
    assert second_clone.get_node_data(name)["pre_delay"] == 11
    assert clone.get_node_data(name)["pre_delay"] == 7
    assert not clone.override_next("AgentContractMissing", [])
    assert clone.override_next(name, [])
    assert clone.get_node_data(name)["next"] == []
    assert clone.get_task_job().job_id == context.get_task_job().job_id


def shared_anchor_state(context):
    name = "AgentContractAnchor"
    clone = context.clone()
    assert context.get_anchor(name) is None
    assert clone.set_anchor(name, "节点 A")
    assert context.get_anchor(name) == "节点 A"
    assert context.set_anchor(name, "节点 B")
    assert clone.get_anchor(name) == "节点 B"
    assert clone.set_anchor(name, "")
    assert context.get_anchor(name) is None


def shared_hit_counts_and_task_details(context):
    entry, target = "AgentContractSubtask", "AgentContractHit"
    clone = context.clone()
    assert clone.override_pipeline(
        {
            entry: {"next": [target], "pre_delay": 0, "post_delay": 0},
            target: {"pre_delay": 0, "post_delay": 0},
        }
    )
    assert context.get_hit_count(target) == 0
    detail = clone.run_task(entry)
    assert detail is not None and detail.status.succeeded
    assert detail.entry == entry
    assert detail.task_id != context.get_task_job().job_id
    assert [node.name for node in detail.nodes] == [entry, target]
    assert all(node.completed for node in detail.nodes)
    assert context.get_hit_count(target) == 1

    tasker = context.tasker
    fetched = tasker.get_task_detail(detail.task_id)
    assert fetched is not None and fetched.status.succeeded
    assert [node.node_id for node in fetched.nodes] == [node.node_id for node in detail.nodes]
    latest = tasker.get_latest_node(target)
    assert latest is not None and latest.node_id == detail.nodes[-1].node_id
    fetched_node = tasker.get_node_detail(latest.node_id)
    assert fetched_node is not None and fetched_node.completed
    assert fetched_node.name == target
    assert fetched_node.recognition is not None and fetched_node.recognition.hit
    assert fetched_node.action is not None and fetched_node.action.success
    assert clone.clear_hit_count(target)
    assert context.get_hit_count(target) == 0


def missing_objects(context):
    missing = "AgentContractMissing"
    image = numpy.zeros((20, 30, 3), dtype=numpy.uint8)
    assert context.get_node_data(missing) is None
    assert context.get_node_object(missing) is None
    assert context.run_recognition(missing, image) is None
    assert context.run_action(missing) is None
    tasker = context.tasker
    assert tasker.get_task_detail(0) is None
    assert tasker.get_node_detail(0) is None
    assert tasker.get_recognition_detail(0) is None
    assert tasker.get_action_detail(0) is None
    assert tasker.get_wait_freezes_detail(0) is None
    assert tasker.get_latest_node(missing) is None
    assert tasker.resource.get_node_data(missing) is None


def recognition_results(context):
    image = numpy.zeros((20, 30, 3), dtype=numpy.uint8)
    hit = context.run_recognition_direct(JRecognitionType.DirectHit, JDirectHit(roi=(2, 3, 5, 7)), image)
    assert hit is not None and hit.reco_id != 0 and hit.hit
    assert tuple(hit.box) == (2, 3, 5, 7)
    assert hit.algorithm == "DirectHit"
    fetched = context.tasker.get_recognition_detail(hit.reco_id)
    assert fetched is not None and fetched.hit and tuple(fetched.box) == (2, 3, 5, 7)

    miss = context.run_recognition_direct(
        JRecognitionType.ColorMatch,
        JColorMatch(lower=[[255, 255, 255]], upper=[[255, 255, 255]]),
        image,
    )
    assert miss is not None and miss.reco_id != 0 and not miss.hit
    assert miss.box is None and miss.algorithm == "ColorMatch"
    fetched_miss = context.tasker.get_recognition_detail(miss.reco_id)
    assert fetched_miss is not None and not fetched_miss.hit
    assert fetched_miss.box is None


def recognition_node_submission(context):
    from agent_test_utils import record_sink_event

    image = numpy.zeros((20, 30, 3), dtype=numpy.uint8)
    job = context.tasker.post_recognition_node(
        "AgentContractProbe", image,
        {"AgentContractProbe": {"recognition": "ColorMatch", "lower": [255, 255, 255],
                                "upper": [255, 255, 255], "inverse": True,
                                "action": "Click", "next": ["AgentContractForbidden"]},
         "AgentContractForbidden": {"action": "Click"}},
    )
    assert job.job_id != 0
    # The owning Tasker worker is waiting for this Custom callback to return.
    # The host checks the queued result after this callback, never wait here.
    record_sink_event(f"remote_probe:{job.job_id}")


def action_results(context):
    action = context.run_action_direct(JActionType.DoNothing, JDoNothing())
    assert action is not None and action.action_id != 0 and action.success
    assert action.action == JActionType.DoNothing
    failed = context.run_action_direct(JActionType.Click, JClick(target="AgentContractMissingTarget"))
    assert failed is not None and failed.action_id != 0 and not failed.success
    assert failed.action == JActionType.Click
    fetched = context.tasker.get_action_detail(failed.action_id)
    assert fetched is not None and not fetched.success
    assert fetched.action == failed.action


def resource_overrides_and_images(context):
    resource = context.tasker.resource
    name = "AgentContractResource"
    assert resource.loaded
    assert resource.override_pipeline({name: {"pre_delay": 23}})
    assert resource.get_node_data(name)["pre_delay"] == 23
    assert context.get_node_data(name)["pre_delay"] == 23
    assert name in resource.node_list
    assert resource.override_next(name, [])
    assert resource.get_node_data(name)["next"] == []

    rng = numpy.random.default_rng(42)
    image = rng.integers(0, 256, (40, 60, 3), dtype=numpy.uint8)
    template = image[9:18, 13:24].copy()
    template_name = "agent-contract.png"
    assert resource.override_image(template_name, template)
    clone = context.clone()
    template_node = "AgentContractTemplate"
    assert clone.override_pipeline(
        {
            template_node: {
                "recognition": "TemplateMatch",
                "template": template_name,
                "threshold": 0.99,
            }
        }
    )
    result = clone.run_recognition(template_node, image)
    assert result is not None and result.hit
    assert tuple(result.box) == (13, 9, 11, 9)

    # A context image override must shadow the resource image across the transport.
    alternate = image[25:34, 37:48].copy()
    assert clone.override_image(template_name, alternate)
    result = clone.run_recognition(template_node, image)
    assert result is not None and result.hit
    assert tuple(result.box) == (37, 25, 11, 9)


def controller_image_round_trip(context):
    controller = context.tasker.controller
    assert controller.connected
    assert controller.uuid
    assert controller.set_screenshot_use_raw_size(True)
    try:
        job = controller.post_screencap().wait()
        assert job.succeeded
        image = job.get()
        assert image is not None and image.ndim == 3 and image.shape[2] == 3
        assert image.dtype == numpy.uint8
        numpy.testing.assert_array_equal(controller.cached_image, image)
        assert controller.resolution == (image.shape[1], image.shape[0])
        assert controller.info["type"] == "dbg"
    finally:
        assert controller.set_screenshot_use_raw_size(False)


def tasker_capture_options(context):
    tasker = context.tasker
    image = numpy.zeros((24, 32, 3), dtype=numpy.uint8)
    assert tasker.set_reco_image_raw(True)
    assert tasker.set_reco_image_draws(False)
    assert tasker.set_reco_image_cache_count(4)
    assert tasker.set_reco_image_cache_bytes(1024 * 1024)
    assert tasker.set_capture_config(True)
    detail = context.run_recognition_direct(JRecognitionType.DirectHit, JDirectHit(), image)
    assert detail is not None and detail.raw_image is not None
    numpy.testing.assert_array_equal(detail.raw_image, image)
    first_id = detail.reco_id
    assert tasker.get_recognition_image_status(first_id) == "available"
    assert tasker.get_recognition_image_status(0) is None

    assert tasker.set_reco_image_cache_bytes(1)
    assert tasker.get_recognition_image_status(first_id) == "evicted"
    detail = context.run_recognition_direct(JRecognitionType.DirectHit, JDirectHit(), image)
    assert detail is not None and detail.raw_image.size == 0
    assert tasker.get_recognition_image_status(detail.reco_id) == "budget_exceeded"

    assert tasker.set_reco_image_raw(False)
    assert tasker.set_capture_config(False)
    assert tasker.set_reco_image_cache_bytes(64 * 1024 * 1024)
    assert tasker.set_reco_image_cache_count(4096)


REMOTE_CONTRACT_CASES = (
    context_clone_isolation,
    shared_anchor_state,
    shared_hit_counts_and_task_details,
    missing_objects,
    recognition_results,
    recognition_node_submission,
    action_results,
    resource_overrides_and_images,
    controller_image_round_trip,
    tasker_capture_options,
)
