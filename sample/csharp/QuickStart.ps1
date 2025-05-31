$n = [int](Read-Host "(1) Main`n(2) PI`n(3) Agent`n(4) CSBinding README Example`nSelect")
$path = @('1.main.csx', '2.pi_cli.csx', '3.AgentMain.csx', "4.CSBinding README Example.csx")[$n - 1]
if ($n -lt 1 -or $n -gt 4) {
    Write-Error "Invalid selection."
    exit 1
}

cd "$PSScriptRoot"
$ENV:DOTNET_SCRIPT_CACHE_LOCATION = "$PSScriptRoot\.cache"

dotnet tool restore
dotnet script "$path" --sources https://api.nuget.org/v3/index.json --sources https://maaxyz.github.io/pkg/nuget/index.json