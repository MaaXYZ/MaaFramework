#!/usr/bin/env dotnet-script
#nullable enable

#r "nuget: Maa.Framework.Binding.Native, 4.2.0"

using MaaFramework.Binding;
using MaaFramework.Binding.Custom;

var commandLineArgs = Environment.GetCommandLineArgs();
if (commandLineArgs.Length < 4)
{
    Console.WriteLine("Call AgentMain.cs instead of this file.");
    return 1;
}

var socketId = commandLineArgs[^1];
var userPath = commandLineArgs[^2];
var dllPath = commandLineArgs[^3];

MaaAgentServer.Current
    .WithIdentifier(socketId)
    .WithNativeLibrary(dllPath)
    .WithToolkitConfig_InitOption(userPath)
    .Register(new MyRec())
    .Register(new MyAct())
    .StartUp()
    .Join()
    .ShutDown();

internal sealed class MyRec : IMaaCustomRecognition
{
    public string Name { get; set; } = "TestRecognition";

    public bool Analyze(in IMaaContext context, in AnalyzeArgs args, in AnalyzeResults results)
    {
        Console.WriteLine("{0} Called", Name);

        return results.Box.TrySetValues(0, 0, 100, 100)
            && results.Detail.TrySetValue("Hello Client!");
    }
}

internal sealed class MyAct : IMaaCustomAction
{
    public string Name { get; set; } = "TestAction";

    public bool Run(in IMaaContext context, in RunArgs args, in RunResults results)
    {
        Console.WriteLine("{0} Called", Name);
        Console.WriteLine("recognition detail: {0}", args.RecognitionDetail);
        Console.WriteLine("custom action param: {0}", args.ActionParam);

        return true;
    }
}
