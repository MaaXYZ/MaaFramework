import sys

from maa.agent.agent_server import AgentServer
from maa.custom_recognition import CustomRecognition
from maa.custom_action import CustomAction
from maa.context import Context


def main():

    socket_id = sys.argv[-1]

    AgentServer.start_up(socket_id)
    AgentServer.join()
    AgentServer.shut_down()


@AgentServer.custom_recognition("MyRecongition")
class MyRecongition(CustomRecognition):

    def analyze(
        self,
        context: Context,
        argv: CustomRecognition.AnalyzeArg,
    ) -> CustomRecognition.AnalyzeResult:
        reco_detail = context.run_recognition(
            "MyCustomOCR",
            argv.image,
            pipeline_override={"MyCustomOCR": {"roi": [100, 100, 200, 300]}},
        )

        # context is a reference, will override the pipeline for whole task
        context.override_pipeline({"MyCustomOCR": {"roi": [1, 1, 114, 514]}})
        # context.run_recognition ...

        # make a new context to override the pipeline, only for itself
        new_context = context.clone()
        new_context.override_pipeline({"MyCustomOCR": {"roi": [100, 200, 300, 400]}})
        reco_detail = new_context.run_recognition("MyCustomOCR", argv.image)

        click_job = context.tasker.controller.post_click(10, 20)
        click_job.wait()

        context.override_next(argv.node_name, ["TaskA", "TaskB"])

        return CustomRecognition.AnalyzeResult(
            box=(0, 0, 100, 100), detail="Hello World!"
        )


@AgentServer.custom_action("MyCustomAction")
class MyCustomAction(CustomAction):

    def run(
        self,
        context: Context,
        argv: CustomAction.RunArg,
    ) -> bool:
        print("MyCustomAction is running!")
        return True


if __name__ == "__main__":
    main()
