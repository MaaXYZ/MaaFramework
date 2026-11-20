"""Agent-side test controller, also runnable as a controller-only AgentServer."""

from pathlib import Path
import sys
import time


def register_test_controller():
    from maa.agent.agent_server import AgentServer

    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
    from recording_controller import RecordingController

    @AgentServer.custom_controller("RecordingDevice")
    class AgentRecordingController(RecordingController):
        def __init__(self):
            super().__init__(raw_size=False)
            self.delay_next_frame = False

        def input_text(self, text):
            self.delay_next_frame = text == "delay-frame"
            return super().input_text(text) and bool(text)

        def screencap(self):
            if self.delay_next_frame:
                self.delay_next_frame = False
                time.sleep(1.5)
                return 255 - self.image
            return super().screencap()

        def get_custom_info(self):
            return {"calls": self.calls}

    assert not AgentServer.register_custom_controller("RecordingDevice", AgentRecordingController())
    assert not AgentServer.register_custom_controller("", AgentRecordingController())


if __name__ == "__main__":
    sys.path.insert(0, sys.argv[1])
    from maa.agent.agent_server import AgentServer
    from agent_test_utils import signal_server_ready

    register_test_controller()
    assert AgentServer.start_up(sys.argv[2])
    signal_server_ready()
    AgentServer.join()
    AgentServer.shut_down()
