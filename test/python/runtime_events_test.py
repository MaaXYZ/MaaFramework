"""Binding decoding for additive runtime events, including older payloads."""

import ctypes
from unittest.mock import patch

from maa.context import ContextEventSink
from maa.event_sink import EventSink, NotificationType


@patch("maa.context.Context")
def test_runtime_event_bindings(_context):
    received = []

    class Sink(ContextEventSink):
        def on_node_next_list(self, context, noti_type, detail):
            received.append((noti_type, detail))

        def on_context_task(self, context, noti_type, detail):
            received.append((noti_type, detail))

        def on_node_transition(self, context, details):
            received.append(details)

    sink = Sink()
    payload = {
        "task_id": 10,
        "name": "A",
        "list": [{"name": "B", "jump_back": False, "anchor": False}],
        "focus": None,
    }
    # Context construction is mocked: only message decoding is exercised.
    handle = ctypes.c_void_p(1)
    sink._on_raw_notification(handle, "Node.NextList.Succeeded", payload)
    assert received[-1][1].list_id is None
    payload.update({
        "context_id": 12, "parent_task_id": 9, "parent": {"kind": "node", "id": 25},
        "list_id": 50, "node_id": 25, "round": 2, "list_type": "on_error", "reason": "cancelled",
    })
    sink._on_raw_notification(handle, "Node.NextList.Cancelled", payload)
    kind, detail = received[-1]
    assert kind == NotificationType.Cancelled
    assert (detail.context_id, detail.parent_task_id, detail.list_id, detail.round) == (12, 9, 50, 2)
    assert detail.list_type == "on_error" and detail.parent == {"kind": "node", "id": 25}
    assert detail.reason == "cancelled"
    sink._on_raw_notification(handle, "Context.Task.Starting", {
        "task_id": 11, "entry": "Child", "parent_task_id": 10,
    })
    assert received[-1][1].parent_task_id == 10
    sink._on_raw_notification(handle, "Node.Transition", {"reason": "jump_back"})
    assert received[-1] == {"reason": "jump_back"}
    assert EventSink._notification_type("Unknown.Cancelled") == NotificationType.Cancelled
