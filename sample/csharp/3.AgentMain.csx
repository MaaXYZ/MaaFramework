#!/usr/bin/env dotnet-script
#nullable enable

#r "nuget: Maa.Framework, 4.2.0"

using System.Diagnostics;
using MaaFramework.Binding;

var maa = new MaaTasker
{
    Controller = MaaToolkit.Shared.AdbDevice.Find()[0].ToAdbController(),
    Resource = new MaaResource(),
    DisposeOptions = DisposeOptions.All,
};
if (!maa.IsInitialized)
    throw new InvalidOperationException("Failed to init tasker.");

var agent = MaaAgentClient.Create(maa.Resource);
using (var cts = new System.Threading.CancellationTokenSource(10 * 1000))
{
    if (!agent.LinkStart(StartupAgentServer, cts.Token))
        throw new InvalidOperationException("Failed to connect.");
}

var ppover = """
    {
        "中文字符测试": {
            "recognition": "Custom",
            "custom_recognition": "TestRecognition",
            "custom_recognition_param": { "RecognitionParam": true },
            "action": "Custom",
            "custom_action": "TestAction",
            "custom_action_param": { "ActionParam": true }
        }
    }
    """;
Console.WriteLine(ppover);

var detail = maa
    .AppendTask("中文字符测试", ppover)
    .WaitFor(MaaJobStatus.Succeeded)
    .QueryTaskDetail()
    ?? throw new InvalidOperationException("Failed to pipeline.");
Console.WriteLine($"pipeline detail: {detail}");
Console.WriteLine($"MyRec detail: {detail.QueryRecognitionDetail(maa, 1)?.Detail}");

Console.Write("Press any key to exit:");
Console.ReadKey();

agent.LinkStop();
// The agent server process will be killed when Dispose() is called.
// Uses LinkStart() or LinkStartUnlessProcessExit(Process, CancellationToken)
// if you do not want the agent to control the life cycle of the process.
agent.Dispose();
maa.Dispose();


static Process? StartupAgentServer(string identifier, string nativeAssemblyDirectory)
{
    var file = "3.AgentChild.csx";
    var paths = new[]
    {
        file,
        "Agent/" + file,
        "csharp/Agent/" + file,
        "sample/csharp/Agent/" + file,
    };

    file = paths.First(File.Exists);
    var userPath = Path.Combine(Environment.CurrentDirectory, ".cache");
    return Process.Start(new ProcessStartInfo(
        "dotnet", $"script {file} {nativeAssemblyDirectory} {userPath} {identifier}")
    {
        UseShellExecute = true,
    });
}
