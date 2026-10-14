#!/usr/bin/env bash
# Native CI links host C libraries but keeps MaaDeps libc++ and its glibc sysroot.
# Expose package headers without exposing host bits/ or sys/, which would mix
# two glibc header versions inside the same translation unit.
set -euo pipefail
output="$1"
mkdir -p "$output/include" "$output/pkgconfig"
output="$(cd "$output" && pwd)"
multiarch="$(cc -print-multiarch)"
for package in libavcodec libavutil libswscale libswresample libavformat libavfilter libavdevice libpostproc curl openssl; do
    for root in /usr/include "/usr/include/$multiarch"; do
        if [[ -d "$root/$package" ]]; then
            mkdir -p "$output/include/$package"
            cp -a "$root/$package/." "$output/include/$package/"
        fi
    done
done
for header in libavcodec/avcodec.h libavutil/frame.h libswscale/swscale.h curl/curl.h openssl/ssl.h openssl/opensslconf.h; do
    test -f "$output/include/$header"
done
for package in libavcodec libavutil libswscale libswresample libavformat libavfilter libavdevice libpostproc libcurl openssl libssl libcrypto; do
    if pkg-config --exists "$package"; then
        pcdir="$(pkg-config --variable=pcfiledir "$package")"
        sed "s|^includedir=.*|includedir=$output/include|" "$pcdir/$package.pc" > "$output/pkgconfig/$package.pc"
    fi
done

flags="$(PKG_CONFIG_PATH="$output/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}" pkg-config --cflags libavcodec libavutil libswscale libcurl)"
if [[ "$flags" == *"-I/usr/include/$multiarch"* ]]; then
    echo "Host multiarch include root remains in protocol compiler flags: $flags" >&2
    exit 1
fi
printf '%s\n' "Isolated native protocol compiler flags: $flags"
