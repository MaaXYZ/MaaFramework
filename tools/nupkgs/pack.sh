#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

version="$1"

curl -fsSL -O https://raw.githubusercontent.com/MaaAssistantArknights/design/main/logo/maa-logo_128x128.png
for nuspec in Maa.Framework.Runtime*.nuspec; do
    # extract RID from filename:
    #     Maa.Framework.Runtime.<rid>.nuspec -> <rid> Maa.Framework.Runtime.<rid>
    #     Maa.Framework.Runtimes.nuspec      -> Maa.Framework.Runtimes
    pid="${nuspec%.nuspec}"
    rid="${pid#Maa.Framework.Runtime.}"
    if [ "${pid}" != "Maa.Framework.Runtimes" ]; then
        sed "s/__PID__/${pid}/g; s/__RID__/${rid}/g" "buildTransitive/template.targets" > "buildTransitive/${pid}.targets"
    fi

    # Prepend Head.nuspec and append Tail.nuspec
    cat Head.nuspec "${nuspec}" Tail.nuspec > temp && mv temp "${nuspec}"

    nuget pack "${nuspec}" \
        -Verbosity detailed \
        -Properties "version=${version//-post/};year=$(date +%Y);branch=$(git rev-parse --abbrev-ref HEAD);commit=$(git rev-parse HEAD);root_path=..\.."
done