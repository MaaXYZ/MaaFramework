#!/usr/bin/env dotnet-script

#r "nuget: Maa.Framework, 2.1.2"

using MaaFramework.Binding;
using MaaFramework.Binding.Buffers;
using MaaFramework.Binding.Custom;

var userPath = Environment.CurrentDirectory;
var toolkit = new MaaToolkit(init: true, userPath: userPath);
// Equivalent to `toolkit.Config.InitOption(userPath);`
var adbDevices = toolkit.AdbDevice.Find();
if (adbDevices.MaaSizeCount < 1)
    throw new InvalidOperationException("No ADB device found.");

// Remember to dispose tasker, controller and resource.
// The `using` statement ensures the correct disposing tasker.
var maa = new MaaTasker()
{
    // For demo, we just use the first device.
    Controller = adbDevices[0].ToAdbController(screencapMethods: AdbScreencapMethods.Encode),
    Resource = new MaaResource("../../sample/resource"),
    // For demo, we dispose controller and resource on disposing tasker.
    DisposeOptions = DisposeOptions.All,
    // we don't need to assign values to Toolkit and Utility,
    // which are designed to be stateless.
    Toolkit = toolkit,
};
maa.Callback += (s, e) =>
{
    Console.WriteLine("Callback({0}): {1}", e.Message, e.Details);
};
maa.Utility.SetOptionStdoutLevel(LoggingLevel.Info);

// By default, Controller and Resource is enabled LinkOption.Start and CheckStatusOption.ThrowIfNotSucceeded.
// If self-control is required, we can write codes as follows.
// Controller = adbDevices[0].ToAdbController(link: LinkOption.Start, check: CheckStatusOption.None),
// Resource = new MaaResource(check: CheckStatusOption.None, "../../sample/resource"),

if (!maa.Initialized)
    throw new InvalidOperationException("Failed to init MAA.");

maa.Resource.Register(new MyRecongition());
var job = maa.AppendPipeline("StartUpAndClickButton");
job.Wait().ThrowIfNot(MaaJobStatus.Succeeded);
var taskDetail = job.QueryTaskDetail();

// do something with taskDetail

maa.Dispose();
Console.WriteLine("Done");

internal sealed class MyRecongition : IMaaCustomRecognition
{
    public string Name { get; set; } = "MyRec";

    public bool Analyze(in IMaaContext context, in AnalyzeArgs args, in AnalyzeResults results)
    {
        var recognitionDetail = context.RunRecognition(
            "MyCustomOCR",
            """{"MyCustomOCR": {"roi": [100, 100, 200, 300]}}""",
            image: args.Image);

        // context is a reference, will override the pipeline for whole task
        context.OverridePipeline("""{"MyCustomOCR": {"roi": [1, 1, 114, 514]}}""");
        // context.RunRecognition ...

        // make a new context to override the pipeline, only for itself
        var newContext = (IMaaContext)context.Clone();
        newContext.OverridePipeline("""{"MyCustomOCR": {"roi": [100, 200, 300, 400]}}""");
        recognitionDetail = newContext.RunRecognition("MyCustomOCR", "{}", args.Image);

        var clickJob = context.Tasker.Controller.Click(10, 20);
        clickJob.Wait().ThrowIfNot(MaaJobStatus.Succeeded);

        context.OverrideNext(args.TaskName, ["TaskA", "TaskB"]);

        results.Box.SetValues(0, 0, 100, 100);
        results.Detail.SetValue("Hello World!");
        return true;
    }
}
