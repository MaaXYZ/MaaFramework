#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

version="$1"

curl -fsSL -O https://raw.githubusercontent.com/MaaAssistantArknights/design/main/logo/maa-logo_128x128.png
for NUSPEC in Maa.Framework.Runtime*.nuspec; do
    # extract RID from filename:
    #     Maa.Framework.Runtime.<rid>.nuspec -> <rid>
    #     Maa.Framework.Runtimes.nuspec      -> Maa.Framework.Runtimes
    rid="${NUSPEC#Maa.Framework.Runtime.}"
    rid="${rid%.nuspec}"
    if [ "${rid}" != "Maa.Framework.Runtimes" ]; then
        sed "s/__RID__/${rid}/g" "buildTransitive/template.targets" > "buildTransitive/Maa.Framework.Runtime.${rid}.targets"
    fi

    # Prepend Head.nuspec and append Tail.nuspec
    cat Head.nuspec "${NUSPEC}" Tail.nuspec > temp && mv temp "${NUSPEC}"

    nuget pack "${NUSPEC}" \
        -Verbosity detailed \
        -Properties "version=${version//-post/};year=$(date +%Y);branch=$(git rev-parse --abbrev-ref HEAD);commit=$(git rev-parse HEAD);root_path=..\.."
done