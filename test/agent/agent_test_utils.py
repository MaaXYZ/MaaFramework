import os
from pathlib import Path
import subprocess
import tempfile
import time
from typing import Optional
import uuid


SERVER_READY_FILE_ENV = "MAAFW_AGENT_SERVER_READY_FILE"
SINK_REPORT_FILE_ENV = "MAAFW_AGENT_SINK_REPORT_FILE"
CONNECT_TIMEOUT_MS = 5000
PROCESS_TIMEOUT_SECONDS = 10


def create_marker_path(name: str) -> Path:
    return Path(tempfile.gettempdir()) / f"maafw-{name}-{uuid.uuid4().hex}"


def signal_server_ready() -> None:
    ready_file = os.environ.get(SERVER_READY_FILE_ENV)
    if ready_file:
        Path(ready_file).write_text("ready", encoding="utf-8")


def record_sink_event(sink_type: str) -> None:
    report_file = os.environ.get(SINK_REPORT_FILE_ENV)
    if not report_file:
        return

    with Path(report_file).open("a", encoding="utf-8") as report:
        print(sink_type, file=report, flush=True)


def start_agent_server(
    command: list[str], sink_report_file: Optional[Path] = None
) -> subprocess.Popen:
    ready_file = create_marker_path("agent-ready")
    environment = os.environ.copy()
    environment[SERVER_READY_FILE_ENV] = str(ready_file)
    if sink_report_file:
        environment[SINK_REPORT_FILE_ENV] = str(sink_report_file)

    child_process = subprocess.Popen(command, env=environment)
    deadline = time.monotonic() + PROCESS_TIMEOUT_SECONDS
    try:
        while time.monotonic() < deadline:
            if ready_file.exists():
                return child_process
            if child_process.poll() is not None:
                raise RuntimeError(
                    f"AgentServer exited before readiness with code {child_process.returncode}"
                )
            time.sleep(0.05)

        raise TimeoutError("AgentServer did not become ready in time")
    except Exception:
        stop_agent_server(child_process)
        raise
    finally:
        ready_file.unlink(missing_ok=True)


def stop_agent_server(child_process: subprocess.Popen) -> None:
    if child_process.poll() is not None:
        return

    child_process.terminate()
    try:
        child_process.wait(timeout=PROCESS_TIMEOUT_SECONDS)
    except subprocess.TimeoutExpired:
        child_process.kill()
        child_process.wait(timeout=PROCESS_TIMEOUT_SECONDS)


def connect_agent_server(
    agent,
    command: list[str],
    sink_report_file: Optional[Path] = None,
    max_attempts: int = 3,
) -> subprocess.Popen:
    assert agent.set_timeout(CONNECT_TIMEOUT_MS)
    try:
        for attempt in range(1, max_attempts + 1):
            child_process = start_agent_server(command, sink_report_file)
            try:
                connected = agent.connect()
            except Exception:
                stop_agent_server(child_process)
                raise

            if connected:
                return child_process

            stop_agent_server(child_process)
            print(f"AgentServer connection attempt {attempt}/{max_attempts} failed")
    finally:
        assert agent.set_timeout(-1)

    raise RuntimeError(f"AgentServer connection failed after {max_attempts} attempts")


def assert_sink_events(report_file: Path) -> None:
    expected = {"resource", "controller", "tasker"}
    actual = (
        set(report_file.read_text(encoding="utf-8").splitlines())
        if report_file.exists()
        else set()
    )
    report_file.unlink(missing_ok=True)
    assert expected <= actual, (
        f"missing forwarded sink events: expected {expected}, got {actual}"
    )


def run_connected_agent_test(
    agent,
    command: list[str],
    sink_report_file: Path,
    resource,
    tasker,
    pipeline_path: Path,
) -> None:
    child_process = connect_agent_server(agent, command, sink_report_file)
    try:
        assert agent.connected
        assert agent.alive

        reco_list = agent.custom_recognition_list
        action_list = agent.custom_action_list
        print(f"agent.custom_recognition_list: {reco_list}")
        print(f"agent.custom_action_list: {action_list}")
        assert "MyRec" in reco_list
        assert "MyAct" in action_list

        resource.post_pipeline(pipeline_path).wait()
        pipeline_override = {
            "Entry": {"next": "Rec"},
            "Rec": {
                "recognition": "Custom",
                "custom_recognition": "MyRec",
                "action": "Custom",
                "custom_action": "MyAct",
                "custom_action_param": "哈哈哈(*´▽｀)ノノ",
            },
        }
        detail = tasker.post_task("Entry", pipeline_override).wait().get()
        if not detail:
            raise RuntimeError("pipeline failed")

        print(f"pipeline detail: entry={detail.entry}, status={detail.status}")
        print(f"pipeline nodes count: {len(detail.nodes)}")
        if detail.nodes:
            node = detail.nodes[0]
            print(f"  first node: {node.name}")

            node_detail = tasker.get_node_detail(node.node_id)
            if node_detail:
                print(
                    f"  node_detail: name={node_detail.name}, completed={node_detail.completed}"
                )

            if node.recognition:
                reco_detail = tasker.get_recognition_detail(node.recognition.reco_id)
                if reco_detail:
                    print(
                        f"  reco_detail: name={reco_detail.name}, algorithm={reco_detail.algorithm}"
                    )

            if node.action:
                action_detail = tasker.get_action_detail(node.action.action_id)
                if action_detail:
                    print(
                        f"  action_detail: name={action_detail.name}, success={action_detail.success}"
                    )

        assert agent.disconnect()
        child_process.wait(timeout=PROCESS_TIMEOUT_SECONDS)
        assert child_process.returncode == 0
        assert_sink_events(sink_report_file)
    finally:
        try:
            if agent.connected:
                assert agent.set_timeout(CONNECT_TIMEOUT_MS)
                try:
                    agent.disconnect()
                finally:
                    assert agent.set_timeout(-1)
        finally:
            try:
                stop_agent_server(child_process)
            finally:
                sink_report_file.unlink(missing_ok=True)
