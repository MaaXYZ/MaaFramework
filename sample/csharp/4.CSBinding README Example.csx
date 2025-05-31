#!/usr/bin/env dotnet-script
#nullable enable

#r "nuget: Maa.Framework, 4.2.0"

using MaaFramework.Binding;
using MaaFramework.Binding.Buffers;
using MaaFramework.Binding.Custom;

QuickStart();
Console.Write("Press any key to exit:");
Console.ReadKey();

void QuickStart()
{
// ### Code Example
// using MaaFramework.Binding;

MaaToolkit.Shared.Config.InitOption(".cache");

var devices = MaaToolkit.Shared.AdbDevice.Find();
if (devices.IsEmpty)
    throw new InvalidOperationException();

using var maa = new MaaTasker
{
    Controller = devices[0].ToAdbController(),
    Resource = new MaaResource("../../src/MaaFramework.Binding.UnitTests/SampleResource"),
    DisposeOptions = DisposeOptions.All,
};

if (!maa.IsInitialized)
    throw new InvalidOperationException();

maa.AppendTask("EmptyNode")
   .Wait()
   .ThrowIfNot(MaaJobStatus.Succeeded);

Console.WriteLine("EmptyNode Completed");


// #### Custom
// using MaaFramework.Binding.Buffers;
// using MaaFramework.Binding.Custom;

var nodeName = "MyCustomTask";
var param = $$"""
{
  "{{nodeName}}": {
      "recognition": "Custom",
      "custom_recognition": "MyRec",
      "custom_recognition_param": {
          "my_rec_key": "my_rec_value"
      },
      "action": "Custom",
      "custom_action": "MyAct",
      "custom_action_param": {
          "my_act_key": "my_act_value"
      }
  }
}
""";

// Register custom components
maa.Resource.Register(new MyRec());
maa.Resource.Register(new MyAct());
maa.AppendTask(nodeName, param)
    .Wait()
    .ThrowIfNot(MaaJobStatus.Succeeded);

}

internal sealed class MyRec : IMaaCustomRecognition
{
    public string Name { get; set; } = nameof(MyRec);
    public bool Analyze(in IMaaContext context, in AnalyzeArgs args, in AnalyzeResults results)
    {
        Console.WriteLine($"Enter {Name}");
        return results.Box.TrySetValues(0, 0, 100, 100)
            && results.Detail.TrySetValue("Hello World!");
    }
}
internal sealed class MyAct : IMaaCustomAction
{
    public string Name { get; set; } = nameof(MyAct);
    public bool Run(in IMaaContext context, in RunArgs args, in RunResults results)
    {
        Console.WriteLine($"Enter {Name}");
        return true;
    }
}
