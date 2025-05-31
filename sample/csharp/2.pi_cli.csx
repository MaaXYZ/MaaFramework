#!/usr/bin/env dotnet-script
#nullable enable

#r "nuget: Maa.Framework, 4.2.0"

using MaaFramework.Binding;
using MaaFramework.Binding.Buffers;
using MaaFramework.Binding.Custom;

MaaUtility.Shared.SetOption_StdoutLevel(LoggingLevel.Info);
// Register custom action
MaaToolkit.Shared.PI.Register(new MyAction());
// Run MaaPiCli
MaaToolkit.Shared.PI.RunCli("../resource", ".cache", false);


internal sealed class MyAction : IMaaCustomAction
{
    public string Name { get; set; } = "MyAct";

    public bool Run(in IMaaContext context, in RunArgs args, in RunResults results)
    {
        Console.WriteLine($"on MyAction.run, context: {context}, args: {args}");

        context.OverrideNext(args.NodeName, ["TaskA", "TaskB"]);

        using var image = new MaaImageBuffer();
        context.Tasker.Controller.GetCachedImage(image);
        context.Tasker.Controller.Click(100, 100).Wait();

        var RecognitionDetail = context.RunRecognition("Cat", image,
            """{"Cat": {"recognition": "OCR", "expected": "喵喵喵"}}""");
        // if RecognitionDetail xxxx

        return true;
    }
}
