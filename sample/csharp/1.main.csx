#!/usr/bin/env dotnet-script
#nullable enable

#r "nuget: Maa.Framework, 4.2.0"

using MaaFramework.Binding;
using MaaFramework.Binding.Buffers;
using MaaFramework.Binding.Custom;

var userPath = ".cache";
var resourcePath = "../resource";

var toolkit = MaaToolkit.Shared.Config.InitOption(userPath);
// If not found on Windows, try running as administrator.
var adbDevice = MaaToolkit.Shared.AdbDevice.Find()[0];

// Remember to dispose tasker, controller and resource.
// The `using` statement ensures the correct disposing tasker.
var maa = new MaaTasker()
{
    // For demo, we just use the first device.
    Controller = adbDevice.ToAdbController(screencapMethods: AdbScreencapMethods.Encode),
    Resource = new MaaResource(resourcePath),
    // For demo, we dispose controller and resource on disposing tasker.
    DisposeOptions = DisposeOptions.All,
};
maa.Callback += (s, e) =>
{
    Console.WriteLine("Callback({0}): {1}", e.Message, e.Details);
};
maa.Utility.SetOption_StdoutLevel(LoggingLevel.Info);

// By default, Controller and Resource is enabled LinkOption.Start and CheckStatusOption.ThrowIfNotSucceeded.
// If self-control is required, we can write codes as follows.
// Controller = adbDevices[0].ToAdbController(link: LinkOption.Start, check: CheckStatusOption.None),
// Resource = new MaaResource(check: CheckStatusOption.None, "../../sample/resource"),

if (!maa.IsInitialized)
    throw new InvalidOperationException("Failed to init MAA.");

maa.Resource.Register(new MyRecongition());
maa.Resource.Register(new MyAction());

// just an example, use it in json
var job = maa.AppendTask("MyCustomEntry", """
{
    "MyCustomEntry": {
        "action": "custom",
        "custom_action": "MyAction"
    }
}
""");
job.Wait().ThrowIfNot(MaaJobStatus.Succeeded);
var taskDetail = job.QueryTaskDetail();

// do something with taskDetail

maa.Dispose();
Console.WriteLine("Done");
Console.Write("Press any key to exit:");
Console.ReadKey();

internal sealed class MyRecongition : IMaaCustomRecognition
{
    public string Name { get; set; } = "MyRecongition";

    public bool Analyze(in IMaaContext context, in AnalyzeArgs args, in AnalyzeResults results)
    {
        var recognitionDetail = context.RunRecognition("MyCustomOCR", args.Image,
            """{"MyCustomOCR": {"recognition": "OCR", "roi": [100, 100, 200, 300]}}""");

        // context is a reference, will override the pipeline for whole task
        context.OverridePipeline("""{"MyCustomOCR": {"roi": [1, 1, 114, 514]}}""");
        // context.RunRecognition ...

        // make a new context to override the pipeline, only for itself
        var newContext = context.Clone();
        newContext.OverridePipeline("""{"MyCustomOCR": {"roi": [100, 200, 300, 400]}}""");
        recognitionDetail = newContext.RunRecognition("MyCustomOCR", args.Image);

        var clickJob = context.Tasker.Controller.Click(10, 20);
        clickJob.Wait().ThrowIfNot(MaaJobStatus.Succeeded);

        context.OverrideNext(args.NodeName, ["TaskA", "TaskB"]);

        return results.Box.TrySetValues(0, 0, 100, 100)
            && results.Detail.TrySetValue("Hello World!");
    }
}

internal sealed class MyAction : IMaaCustomAction
{
    public string Name { get; set; } = "MyAction";

    public bool Run(in IMaaContext context, in RunArgs args, in RunResults results)
    {
        Console.WriteLine("MyAction is running!");
        return true;
    }
}
