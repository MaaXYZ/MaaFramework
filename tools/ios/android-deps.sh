#!/usr/bin/env bash
#
# tools/ios/android-deps.sh
#
# Build the arm64-android OpenSSL + curl and shared FFmpeg dependency prefix that the
# MaaFramework iOS control unit needs when it is cross-compiled for an Android
# host (MaaDeps ships neither library for Android).
#
# Targets arm64-android only for now. Downloads pinned upstream tarballs
# (sources are never vendored into git), verifies sha256, builds
# OpenSSL, curl and a minimal HEVC/PNG FFmpeg build into the requested prefix, and prints the
# CMake command to configure MaaFramework with those dependencies.
#
# Idempotent: a component whose libraries already exist in the prefix is
# skipped. Re-running is cheap; delete the prefix (or a component's libs) to
# force a rebuild.
#
# Usage:
#   tools/ios/android-deps.sh [-j N] <output-prefix>
#
# Environment:
#   ANDROID_NDK_ROOT / NDK_HOME   required NDK location (first one wins).
#   ANDROID_API                   Bionic API level (default 24; required -
#                                 getifaddrs landed in API 24 and the iOS
#                                 controller enumerates network interfaces).
#   JOBS                          parallelism, same as -j.
#   WORK_DIR                      where tarballs are cached and unpacked
#                                 (default: <output-prefix>/../src).
#
# If DNS is unreliable, export http_proxy/https_proxy (e.g.
# http://127.0.0.1:7890) before running; downloads use curl.

set -euo pipefail

# ---------------------------------------------------------------------------
# Pinned upstream sources (never vendor these into the repository).
# ---------------------------------------------------------------------------
OPENSSL_VERSION="3.5.9"
OPENSSL_URL="https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_VERSION}/openssl-${OPENSSL_VERSION}.tar.gz"
OPENSSL_SHA256="603f5602e2eef00d77fbd429d34dcd5822bb301757a1bc9cdb24c670f1eb859a"

CURL_VERSION="8.23.0"
# Note: the curl tarball basename uses dots (curl-8.23.0.tar.xz), not
# underscores (curl-8_23_0 would be a 404).
CURL_URL="https://curl.se/download/curl-${CURL_VERSION}.tar.xz"
CURL_SHA256="8dadd7d2536a314f982ad05a651b713f426bf7d02cc2c76cffb9796f2218e6c5"

FFMPEG_VERSION="9.0.2"
FFMPEG_URL="https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.xz"
FFMPEG_SHA256="8c3850283eb25fa026482078a04051e0be17347b09ef81a0849bec15a96e002e"

ANDROID_API="${ANDROID_API:-24}"
TRIPLE="aarch64-linux-android"

usage() {
    awk 'found { if (!/^#/) exit; sub(/^# ?/, ""); print }
         /^# Usage:/ { found = 1 }' "$0"
    exit "${1:-0}"
}

log() { printf '[ios-android-deps] %s\n' "$*"; }
die() { printf '[ios-android-deps] ERROR: %s\n' "$*" >&2; exit 1; }

# ---------------------------------------------------------------------------
# Arguments
# ---------------------------------------------------------------------------
JOBS="${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
while [ $# -gt 0 ]; do
    case "$1" in
        -j)
            [ $# -ge 2 ] || die "-j needs a value"
            JOBS="$2"; shift 2 ;;
        -j*)
            JOBS="${1#-j}"; shift ;;
        -h|--help)
            usage 0 ;;
        -*)
            die "unknown option: $1 (see --help)" ;;
        *)
            break ;;
    esac
done
[ $# -eq 1 ] || usage 1

# absolute-ise and create the prefix
mkdir -p "$1"
PREFIX=$(cd "$1" && pwd)
WORK_DIR="${WORK_DIR:-$(dirname "$PREFIX")/src}"
mkdir -p "$WORK_DIR"

# ---------------------------------------------------------------------------
# NDK resolution
# ---------------------------------------------------------------------------
NDK="${ANDROID_NDK_ROOT:-${NDK_HOME:-}}"
[ -n "$NDK" ] || die "set ANDROID_NDK_ROOT or NDK_HOME to the Android NDK directory"
[ -d "$NDK" ] || die "NDK not found at '$NDK' (set ANDROID_NDK_ROOT or NDK_HOME)"
NDK=$(cd "$NDK" && pwd)

# Pick the NDK host tag that actually exists (e.g. darwin-x86_64 NDKs running
# under Rosetta on Apple Silicon hosts) and must contain our API wrapper.
NDK_BIN=""
for candidate in "$NDK"/toolchains/llvm/prebuilt/*/bin; do
    if [ -x "$candidate/${TRIPLE}${ANDROID_API}-clang" ]; then
        NDK_BIN="$candidate"
        break
    fi
done
[ -n "$NDK_BIN" ] || die "NDK clang wrapper '${TRIPLE}${ANDROID_API}-clang' not found under $NDK/toolchains/llvm/prebuilt/*/bin (wrong NDK or ANDROID_API?)"

command -v perl >/dev/null 2>&1 || die "perl is required to configure OpenSSL"
command -v make >/dev/null 2>&1 || die "make is required"

sha256_of() {
    if command -v shasum >/dev/null 2>&1; then shasum -a 256 "$1" | awk '{print $1}'
    else sha256sum "$1" | awk '{print $1}'
    fi
}

fetch_pinned() {
    # $1 url, $2 expected sha256, $3 destination file
    local url="$1" want="$2" dst="$3" tmp
    if [ -f "$dst" ] && [ "$(sha256_of "$dst")" = "$want" ]; then
        log "cache hit (sha256 ok): $(basename "$dst")"
        return 0
    fi
    log "downloading $url"
    tmp="$dst.part"
    curl --fail --location --retry 3 --retry-delay 2 -o "$tmp" "$url"
    local got; got=$(sha256_of "$tmp")
    [ "$got" = "$want" ] || { rm -f "$tmp"; die "sha256 mismatch for $(basename "$dst"): got $got want $want"; }
    mv "$tmp" "$dst"
}

# ---------------------------------------------------------------------------
# OpenSSL (static, arm64-android via the NDK clang wrappers, API $ANDROID_API)
# ---------------------------------------------------------------------------
build_openssl() {
    if [ -f "$PREFIX/lib/libcrypto.a" ] && [ -f "$PREFIX/lib/libssl.a" ] \
       && [ -f "$PREFIX/include/openssl/ssl.h" ]; then
        log "OpenSSL already built at $PREFIX - skipping"
        return 0
    fi
    fetch_pinned "$OPENSSL_URL" "$OPENSSL_SHA256" "$WORK_DIR/openssl-${OPENSSL_VERSION}.tar.gz"

    local src="$WORK_DIR/openssl-${OPENSSL_VERSION}"
    if [ ! -f "$src/Configure" ]; then
        rm -rf "$src"
        tar -xf "$WORK_DIR/openssl-${OPENSSL_VERSION}.tar.gz" -C "$WORK_DIR"
    fi

    log "building OpenSSL $OPENSSL_VERSION for arm64-android (API $ANDROID_API)"
    (
        cd "$src"
        export ANDROID_NDK_ROOT="$NDK"
        # OpenSSL's Configurations/15-android.conf detects the toolchain by
        # probing PATH for the NDK clang/llvm-ar; without this it falls back
        # to looking for the long-removed $triarch-gcc and dies.
        export PATH="$NDK_BIN:$PATH"
        # OpenSSL 3.5.x names the arm64 Android target "android-arm64"
        # (the historical "android-aarch64" spelling no longer exists).
        # --libdir=lib keeps lib/ stable on 64-bit targets for CMake find_* .
        # -D__ANDROID_API__=24: getifaddrs() landed in Bionic API 24 and the
        # iOS controller enumerates interfaces; NDK clang wrapper is picked up
        # automatically from ANDROID_NDK_ROOT.
        perl ./Configure android-arm64 \
            no-shared \
            -D__ANDROID_API__="$ANDROID_API" \
            "--prefix=$PREFIX" \
            --libdir=lib
        make -j"$JOBS"
        make install_sw
    )
}

# ---------------------------------------------------------------------------
# curl (static, linked against the OpenSSL just installed into $PREFIX)
# ---------------------------------------------------------------------------
build_curl() {
    if [ -f "$PREFIX/lib/libcurl.a" ] && [ -f "$PREFIX/include/curl/curl.h" ]; then
        log "curl already built at $PREFIX - skipping"
        return 0
    fi
    [ -f "$PREFIX/lib/libcrypto.a" ] || die "curl needs OpenSSL built first (internal ordering)"
    fetch_pinned "$CURL_URL" "$CURL_SHA256" "$WORK_DIR/curl-${CURL_VERSION}.tar.xz"

    local src="$WORK_DIR/curl-${CURL_VERSION}"
    if [ ! -f "$src/configure" ]; then
        rm -rf "$src"
        tar -xf "$WORK_DIR/curl-${CURL_VERSION}.tar.xz" -C "$WORK_DIR"
    fi

    log "building curl $CURL_VERSION for arm64-android (API $ANDROID_API)"
    (
        cd "$src"
        # Cross with the NDK clang wrappers for API $ANDROID_API; --with-openssl
        # points at this very prefix, --with-ca-path at Android's system trust
        # store. libpsl is GPL-encumbered and pointless for static curl here.
        CC="$NDK_BIN/${TRIPLE}${ANDROID_API}-clang" \
        CXX="$NDK_BIN/${TRIPLE}${ANDROID_API}-clang++" \
        ./configure \
            --host="$TRIPLE" \
            --prefix="$PREFIX" \
            --with-openssl="$PREFIX" \
            --enable-static \
            --disable-shared \
            --disable-threaded-resolver \
            --without-libpsl \
            --without-libidn2 \
            --without-zlib \
            --without-brotli \
            --without-zstd \
            --with-ca-path=/system/etc/security/cacerts
        make -j"$JOBS"
        make install
    )
}

# FFmpeg is shared so consumers can replace the LGPL libraries independently.
build_ffmpeg() {
    local recipe="ffmpeg-$FFMPEG_VERSION arm64-v8a api-$ANDROID_API ndk-$(basename "$NDK") recipe-1"
    local marker="$PREFIX/share/ffmpeg/maafw-build.txt"
    if [ -f "$PREFIX/lib/libavcodec.so" ] && [ -f "$PREFIX/lib/libavutil.so" ] \
       && [ -f "$PREFIX/lib/libswscale.so" ] && [ -f "$PREFIX/include/libavcodec/avcodec.h" ] \
       && [ -f "$marker" ] && [ "$(cat "$marker")" = "$recipe" ]; then
        log "FFmpeg already built at $PREFIX - skipping"
        return 0
    fi
    fetch_pinned "$FFMPEG_URL" "$FFMPEG_SHA256" "$WORK_DIR/ffmpeg-${FFMPEG_VERSION}.tar.xz"
    local src="$WORK_DIR/ffmpeg-${FFMPEG_VERSION}"
    if [ ! -f "$src/configure" ]; then
        tar -xf "$WORK_DIR/ffmpeg-${FFMPEG_VERSION}.tar.xz" -C "$WORK_DIR"
    fi
    local output="$WORK_DIR/ffmpeg-build-arm64-api-$ANDROID_API"
    mkdir -p "$output"
    log "building FFmpeg $FFMPEG_VERSION for arm64-android (API $ANDROID_API)"
    (
        cd "$output"
        "$src/configure" \
            --prefix="$PREFIX" --libdir="$PREFIX/lib" \
            --target-os=android --arch=aarch64 --enable-cross-compile \
            --cc="$NDK_BIN/${TRIPLE}${ANDROID_API}-clang" \
            --cxx="$NDK_BIN/${TRIPLE}${ANDROID_API}-clang++" \
            --ar="$NDK_BIN/llvm-ar" --ranlib="$NDK_BIN/llvm-ranlib" \
            --nm="$NDK_BIN/llvm-nm" --strip="$NDK_BIN/llvm-strip" \
            --sysroot="$(dirname "$NDK_BIN")/sysroot" \
            --disable-autodetect --disable-everything \
            --disable-programs --disable-doc --disable-debug --disable-network \
            --disable-avformat --disable-avdevice --disable-avfilter --disable-swresample \
            --enable-shared --disable-static --enable-pic --enable-pthreads \
            --enable-avcodec --enable-avutil --enable-swscale \
            --enable-decoder=hevc,png --enable-parser=hevc --enable-zlib
        make -j"$JOBS"
        make install
    )
    mkdir -p "$PREFIX/share/ffmpeg"
    cp "$src/COPYING.LGPLv2.1" "$PREFIX/share/ffmpeg/"
    cp "$output/ffbuild/config.log" "$PREFIX/share/ffmpeg/"
    printf '%s\n' "$recipe" > "$marker"
}

build_openssl
build_curl
build_ffmpeg

# ---------------------------------------------------------------------------
# Verification + CMake flags for the caller
# ---------------------------------------------------------------------------
for f in lib/libcrypto.a lib/libssl.a lib/libcurl.a include/curl/curl.h include/openssl/ssl.h lib/libavcodec.so lib/libavutil.so lib/libswscale.so; do
    [ -f "$PREFIX/$f" ] || die "expected artifact missing after build: $PREFIX/$f"
done

log "done. arm64-android dependency prefix ready at: $PREFIX"
cat <<EOF

Run from the MaaFramework repository root (requires Android arm64 MaaDeps):

  cmake -S . -B build/android/arm64-v8a -G "Ninja Multi-Config" \\
    -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \\
    -DANDROID_ABI=arm64-v8a \\
    -DANDROID_PLATFORM=android-$ANDROID_API \\
    -DMAADEPS_TRIPLET=maa-arm64-android \\
    -DWITH_IOS_CONTROLLER=ON \\
    -DOPENSSL_ROOT_DIR="$PREFIX" \\
    -DOPENSSL_INCLUDE_DIR="$PREFIX/include" \\
    -DOPENSSL_CRYPTO_LIBRARY="$PREFIX/lib/libcrypto.a" \\
    -DOPENSSL_SSL_LIBRARY="$PREFIX/lib/libssl.a" \\
    -DCURL_INCLUDE_DIR="$PREFIX/include" \\
    -DCURL_LIBRARY="$PREFIX/lib/libcurl.a" \\
    -DMAA_IOS_FFMPEG_INCLUDE_DIR="$PREFIX/include" \\
    -DMAA_IOS_AVCODEC_LIBRARY="$PREFIX/lib/libavcodec.so" \\
    -DMAA_IOS_AVUTIL_LIBRARY="$PREFIX/lib/libavutil.so" \\
    -DMAA_IOS_SWSCALE_LIBRARY="$PREFIX/lib/libswscale.so"
EOF
