#pragma once

#include "Conf/Conf.h"

#include <string_view>

#include <meojson/json.hpp>

MAA_TOOLKIT_DEVICE_NS_BEGIN

using namespace json::literals;
inline static const json::value kAdbConfig = R"(
{
    "prebuilt": {
        "minicap": {
            "arch": [
                "x86",
                "armeabi-v7a",
                "armeabi"
            ],
            "sdk": [
                31, 29, 28, 27, 26, 25, 24, 23, 22, 21, 19, 18, 17, 16, 15, 14
            ]
        },
        "minitouch": {
            "arch": [
                "x86_64",
                "x86",
                "arm64-v8a",
                "armeabi-v7a",
                "armeabi"
            ]
        },
        "maatouch": {
            "package": "com.shxyke.MaaTouch.App"
        }
    },
    "command": {
        "Devices": [
            "{ADB}",
            "devices"
        ],
        "Connect": [
            "{ADB}",
            "connect",
            "{ADB_SERIAL}"
        ],
        "KillServer": [
            "{ADB}",
            "kill-server"
        ],
        "UUID": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "settings get secure android_id"
        ],
        "Resolution": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "dumpsys window displays | grep -o -E cur=+[^\\ ]+ | grep -o -E [0-9]+"
        ],
        "StartApp": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "am start -n {INTENT}"
        ],
        "StopApp": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "am force-stop {INTENT}"
        ],
        "Click": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "input tap {X} {Y}"
        ],
        "Swipe": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "input swipe {X1} {Y1} {X2} {Y2} {DURATION}"
        ],
        "PressKey": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "input keyevent {KEY}"
        ],
        "ForwardSocket": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "forward",
            "tcp:{FOWARD_PORT}",
            "localabstract:{LOCAL_SOCKET}"
        ],
        "NetcatAddress": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "cat /proc/net/arp | grep : "
        ],
        "ScreencapRawByNetcat": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "exec-out",
            "screencap | nc -w 3 {NETCAT_ADDRESS} {NETCAT_PORT}"
        ],
        "ScreencapRawWithGzip": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "exec-out",
            "screencap | gzip -1"
        ],
        "ScreencapEncode": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "exec-out",
            "screencap -p"
        ],
        "ScreencapEncodeToFile": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "screencap -p > \"/data/local/tmp/{TEMP_FILE}\""
        ],
        "PullFile": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "pull",
            "/data/local/tmp/{TEMP_FILE}",
            "{DST_PATH}"
        ],
        "Abilist": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "getprop ro.product.cpu.abilist | tr -d '\n\r'"
        ],
        "SDK": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "getprop ro.build.version.sdk | tr -d '\n\r'"
        ],
        "Orientation": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "dumpsys input | grep SurfaceOrientation | grep -m 1 -o -E [0-9]"
        ],
        "PushBin": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "push",
            "{BIN_PATH}",
            "/data/local/tmp/{BIN_WORKING_FILE}"
        ],
        "ChmodBin": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "chmod 700 \"/data/local/tmp/{BIN_WORKING_FILE}\""
        ],
        "InvokeBin": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "export LD_LIBRARY_PATH=/data/local/tmp/; \"/data/local/tmp/{BIN_WORKING_FILE}\" {BIN_EXTRA_PARAMS} 2>&1"
        ],
        "InvokeApp": [
            "{ADB}",
            "-s",
            "{ADB_SERIAL}",
            "shell",
            "export CLASSPATH=\"/data/local/tmp/{APP_WORKING_FILE}\"; app_process /data/local/tmp {PACKAGE_NAME}"
        ]
    }
})"_json;

MAA_TOOLKIT_DEVICE_NS_END
