// wait_freezes 的比对依赖 TemplateComparator，而 TemplateComparator 在 patch 无方差（纯色）时
// 会落到 cv::matchTemplate 的退化分支，历史上拿到的是与画面无关的硬编码值。这里用合成画面覆盖
// 纯色 ROI 的静止 / 变化两种场景，避免纯色区域得不到正确的静止判定。
#include "WaitFreezesComparison.h"

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <meojson/json.hpp>
#include <opencv2/opencv.hpp>

#include "MaaFramework/MaaAPI.h"
#include "MaaFramework/MaaMsg.h"

namespace
{

constexpr int kFrameWidth = 320;
constexpr int kFrameHeight = 240;
// 纯色/纹理块故意不贴着画面左边缘，让 patch 是真实截图那样的非连续子矩阵
const cv::Rect kRoi { 272, 200, 32, 33 };

struct FrameSpec
{
    int solid = -1; // >= 0 时 ROI 填该灰度纯色，< 0 时 ROI 填纹理
    unsigned texture_seed = 0;
};

struct ControllerState
{
    std::vector<FrameSpec> script;
    size_t index = 0;
    bool hold_last = false; // true 时播完停在最后一帧，用于构造"变了之后就一直静止"
};

cv::Mat make_frame(const FrameSpec& spec)
{
    cv::Mat frame(kFrameHeight, kFrameWidth, CV_8UC3);
    cv::RNG background(2024);
    background.fill(frame, cv::RNG::UNIFORM, 0, 256);

    cv::Mat roi = frame(kRoi);
    if (spec.solid >= 0) {
        roi.setTo(cv::Scalar(spec.solid, spec.solid, spec.solid));
    }
    else {
        cv::Mat texture(roi.size(), roi.type());
        cv::RNG rng(spec.texture_seed);
        rng.fill(texture, cv::RNG::UNIFORM, 0, 256);
        texture.copyTo(roi);
    }

    return frame;
}

MaaBool ctrl_connect(void* /*trans_arg*/)
{
    return true;
}

MaaBool ctrl_connected(void* /*trans_arg*/)
{
    return true;
}

MaaBool ctrl_request_uuid(void* /*trans_arg*/, MaaStringBuffer* buffer)
{
    return MaaStringBufferSet(buffer, "wait_freezes_comparison");
}

MaaBool ctrl_screencap(void* trans_arg, MaaImageBuffer* buffer)
{
    auto* state = static_cast<ControllerState*>(trans_arg);
    if (!state || state->script.empty()) {
        return false;
    }

    const size_t raw_index = state->hold_last ? std::min(state->index, state->script.size() - 1) : state->index % state->script.size();
    const cv::Mat frame = make_frame(state->script.at(raw_index));
    ++state->index;

    return MaaImageBufferSetRawData(buffer, frame.data, frame.cols, frame.rows, frame.type());
}

struct WaitFreezesCapture
{
    int succeeded = 0;
    int failed = 0;
    MaaRecoId last_reco_id = MaaInvalidId;
};

void wait_freezes_sink(void* /*handle*/, const char* message, const char* details_json, void* trans_arg)
{
    if (!message || !trans_arg) {
        return;
    }

    auto* capture = static_cast<WaitFreezesCapture*>(trans_arg);
    if (std::strcmp(message, MaaMsg_Node_WaitFreezes_Succeeded) == 0) {
        ++capture->succeeded;
    }
    else if (std::strcmp(message, MaaMsg_Node_WaitFreezes_Failed) == 0) {
        ++capture->failed;
    }
    else {
        return;
    }

    auto details = json::parse(details_json ? details_json : "");
    if (details && details->contains("reco_ids")) {
        for (const auto& id : details->at("reco_ids").as_array()) {
            capture->last_reco_id = static_cast<MaaRecoId>(id.as_integer());
        }
    }
}

std::string last_scores(const MaaTasker* tasker, MaaRecoId reco_id)
{
    if (reco_id == MaaInvalidId) {
        return "n/a";
    }

    auto node_name = MaaStringBufferCreate();
    auto algorithm = MaaStringBufferCreate();
    auto detail = MaaStringBufferCreate();
    auto box = MaaRectCreate();
    auto raw = MaaImageBufferCreate();
    auto draws = MaaImageListBufferCreate();
    MaaBool hit = false;
    MaaTaskerGetRecognitionDetail(tasker, reco_id, node_name, algorithm, &hit, box, detail, raw, draws);

    const char* str = MaaStringBufferGet(detail);
    std::string scores = str ? str : "";
    MaaImageListBufferDestroy(draws);
    MaaImageBufferDestroy(raw);
    MaaRectDestroy(box);
    MaaStringBufferDestroy(detail);
    MaaStringBufferDestroy(algorithm);
    MaaStringBufferDestroy(node_name);

    return scores;
}

struct Scenario
{
    std::string name;
    std::vector<FrameSpec> script;
    int method = 5;
    double threshold = 0.95;
    int time = 200;
    int timeout = 3000;
    int rate_limit = 200;
    bool expect_frozen = false;
    bool hold_last = false;
};

bool run_scenario(const Scenario& scenario)
{
    ControllerState state { .script = scenario.script, .hold_last = scenario.hold_last };

    MaaCustomControllerCallbacks callbacks {
        .connect = ctrl_connect,
        .connected = ctrl_connected,
        .request_uuid = ctrl_request_uuid,
        .screencap = ctrl_screencap,
    };

    auto controller = MaaCustomControllerCreate(&callbacks, &state);
    auto resource = MaaResourceCreate();
    auto tasker = MaaTaskerCreate();
    MaaTaskerBindResource(tasker, resource);
    MaaTaskerBindController(tasker, controller);

    // 关掉截图缩放，让 ROI 坐标与实际画面一一对应
    const bool use_raw_size = true;
    MaaControllerSetOption(controller, MaaCtrlOption_ScreenshotUseRawSize, const_cast<bool*>(&use_raw_size), sizeof(use_raw_size));

    // 控制器对截图总会走一次 resize（即使目标尺寸与原始尺寸相同）。重采样在部分平台上会让同一块
    // 纯色区域出现逐帧 ±1 抖动，那样测到的是重采样而不是比较器，所以这里用 INTER_NEAREST 原样透传。
    const int32_t resize_method = cv::INTER_NEAREST;
    MaaControllerSetOption(
        controller,
        MaaCtrlOption_ScreenshotResizeMethod,
        const_cast<int32_t*>(&resize_method),
        sizeof(resize_method));

    MaaControllerWait(controller, MaaControllerPostConnection(controller));

    WaitFreezesCapture capture;
    auto sink_id = MaaTaskerAddContextSink(tasker, &wait_freezes_sink, &capture);

    json::value pipeline_override {
        { "WaitFreezesComparison",
          json::object {
              { "action", "DoNothing" },
              { "post_wait_freezes",
                json::object {
                    { "time", scenario.time },
                    { "timeout", scenario.timeout },
                    { "rate_limit", scenario.rate_limit },
                    { "threshold", scenario.threshold },
                    { "method", scenario.method },
                    { "target", json::array { kRoi.x, kRoi.y, kRoi.width, kRoi.height } },
                } },
          } },
    };

    const std::string override_str = pipeline_override.to_string();
    const auto task_id = MaaTaskerPostTask(tasker, "WaitFreezesComparison", override_str.c_str());
    const auto status = MaaTaskerWait(tasker, task_id);

    // wait_freezes 的返回值不会被 action 采纳，只能通过通知观察
    const bool frozen = capture.succeeded > 0 && capture.failed == 0;
    const bool passed = status == MaaStatus_Succeeded && frozen == scenario.expect_frozen;

    std::cout << (passed ? "[pass] " : "[FAIL] ") << scenario.name << " | method=" << scenario.method << " frozen=" << frozen
              << " expect_frozen=" << scenario.expect_frozen << " | status=" << static_cast<int>(status)
              << " succeeded=" << capture.succeeded << " failed=" << capture.failed
              << " | last=" << last_scores(tasker, capture.last_reco_id) << std::endl;

    MaaTaskerRemoveContextSink(tasker, sink_id);
    MaaTaskerDestroy(tasker);
    MaaResourceDestroy(resource);
    MaaControllerDestroy(controller);

    return passed;
}

} // namespace

bool wait_freezes_comparison()
{
    const std::vector<Scenario> scenarios {
        // 两块相同的纯色 patch 必须判定为静止（method 5 是默认值）
        Scenario {
            .name = "identical solid white",
            .script = { FrameSpec { .solid = 253 } },
            .method = 5,
            .expect_frozen = true,
        },
        // 纯黑同样是无方差 patch，但 SQDIFF_NORMED / CCORR_NORMED 的分母（ΣT²·ΣI²）也退化为 0
        Scenario {
            .name = "identical solid black",
            .script = { FrameSpec { .solid = 0 } },
            .method = 5,
            .expect_frozen = true,
        },
        Scenario {
            .name = "identical solid black",
            .script = { FrameSpec { .solid = 0 } },
            .method = 1,
            .expect_frozen = true,
        },
        Scenario {
            .name = "identical solid black",
            .script = { FrameSpec { .solid = 0 } },
            .method = 3,
            .expect_frozen = true,
        },
        // 纯色 ROI 在两个纯色间来回切换仍然是"画面在变"，不能误报静止
        Scenario {
            .name = "solid white <-> solid gray",
            .script = { FrameSpec { .solid = 253 }, FrameSpec { .solid = 100 } },
            .method = 5,
            .expect_frozen = false,
        },
        // 纯色 vs 有纹理且不相同，仍然是"未静止"
        Scenario {
            .name = "solid white <-> textured",
            .script = { FrameSpec { .solid = 253 }, FrameSpec { .texture_seed = 1 } },
            .method = 5,
            .time = 2000,
            .timeout = 4000,
            .expect_frozen = false,
        },
        // invert_score（method + 10000）会把分数反转成 1 - score，"完全一致"因此对应最差分，
        // 与非退化输入下的行为一致：反转语义是找变化，不能用来等画面静止
        Scenario {
            .name = "identical solid white, inverted method",
            .script = { FrameSpec { .solid = 253 } },
            .method = 10005,
            .expect_frozen = false,
        },
        // 10001 = 反转 + SQDIFF_NORMED：SQDIFF 系原本"越小越接近"，反转后一致对应最高分，
        // 方向必须按去掉 10000 后的 method 判断，否则与同一对纹理 patch 的判定相反
        Scenario {
            .name = "identical solid black, inverted sqdiff",
            .script = { FrameSpec { .solid = 0 } },
            .method = 10001,
            .expect_frozen = true,
        },
        Scenario {
            .name = "solid black <-> solid white, inverted sqdiff",
            .script = { FrameSpec { .solid = 0 }, FrameSpec { .solid = 253 } },
            .method = 10001,
            .expect_frozen = false,
        },
        // wait_freezes 在未命中时会把 pre_image 前滚到当前帧，所以"纯色一侧"这种一次性的退化分数
        // 最多损失一次采样，下一采样即自愈 —— 这也是它无法解释"连续几十次 0.000000"的原因
        Scenario {
            .name = "flat white then static texture (self-heal)",
            .script = { FrameSpec { .solid = 253 }, FrameSpec { .texture_seed = 1 } },
            .method = 5,
            .timeout = 4000,
            .expect_frozen = true,
            .hold_last = true,
        },
        // 对照组：有纹理的 ROI 不受此次改动影响
        Scenario {
            .name = "identical textured",
            .script = { FrameSpec { .texture_seed = 7 } },
            .method = 5,
            .expect_frozen = true,
        },
    };

    bool all_passed = true;
    for (const auto& scenario : scenarios) {
        all_passed = run_scenario(scenario) && all_passed;
    }

    std::cout << (all_passed ? "[pass] all wait_freezes comparison scenarios" : "[FAIL] some wait_freezes comparison scenarios")
              << std::endl;

    return all_passed;
}
