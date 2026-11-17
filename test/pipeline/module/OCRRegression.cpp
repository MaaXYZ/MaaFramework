#include "OCRRegression.h"

#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <meojson/json.hpp>

#include "MaaFramework/MaaAPI.h"
#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/ImageIo.h"

namespace
{

constexpr const char* kTarget = "OCRRegressionTarget";
constexpr const char* kEntry = "OCRRegressionEntry";
constexpr const char* kFallback = "OCRRegressionFallback";

struct RecognitionResult
{
    size_t observations = 0;
    bool hit = false;
    bool task_succeeded = false;
    json::value detail;

    bool succeeded() const
    {
        return task_succeeded && observations == 1 && hit && detail.is_object() && detail.contains("all") && detail.contains("filtered")
               && detail.contains("best") && !detail.at("best").is_null();
    }

    json::value to_json() const
    {
        return {
            { "observations", observations },
            { "hit", hit },
            { "task_succeeded", task_succeeded },
            { "detail", detail },
        };
    }
};

void capture_recognition(void*, const char* message, const char* details_json, void* trans_arg)
{
    if (!message || !details_json) {
        return;
    }
    const std::string_view event(message);
    if (event != MaaMsg_Node_Recognition_Succeeded && event != MaaMsg_Node_Recognition_Failed) {
        return;
    }
    auto details = json::parse(details_json);
    if (!details || details->get("name", std::string()) != kTarget || !details->contains("reco_details")) {
        return;
    }
    const auto& recognition = details->at("reco_details");
    if (!recognition.is_object() || !recognition.contains("detail")) {
        return;
    }
    auto& result = *static_cast<RecognitionResult*>(trans_arg);
    ++result.observations;
    result.hit = event == MaaMsg_Node_Recognition_Succeeded;
    result.detail = recognition.at("detail");
}

MaaBool connected(void*)
{
    return true;
}

MaaBool request_uuid(void*, MaaStringBuffer* buffer)
{
    return MaaStringBufferSet(buffer, "ocr-regression");
}

MaaControllerFeature get_features(void*)
{
    return MaaControllerFeature_None;
}

MaaBool screencap(void* trans_arg, MaaImageBuffer* buffer)
{
    const auto& image = *static_cast<cv::Mat*>(trans_arg);
    return MaaImageBufferSetRawData(buffer, image.data, image.cols, image.rows, image.type());
}

class Fixture
{
public:
    explicit Fixture(cv::Mat image)
        : image_(std::move(image))
    {
        static MaaCustomControllerCallbacks callbacks {
            .connect = connected,
            .connected = connected,
            .request_uuid = request_uuid,
            .get_features = get_features,
            .screencap = screencap,
            .inactive = connected,
        };
        controller_ = MaaCustomControllerCreate(&callbacks, &image_);
        tasker_ = MaaTaskerCreate();
    }

    ~Fixture()
    {
        MaaTaskerDestroy(tasker_);
        MaaControllerDestroy(controller_);
    }

    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;

    bool init(MaaResource* resource)
    {
        bool raw_size = true;
        return controller_ && tasker_
               && MaaControllerSetOption(controller_, MaaCtrlOption_ScreenshotUseRawSize, &raw_size, sizeof(raw_size))
               && MaaControllerWait(controller_, MaaControllerPostConnection(controller_)) == MaaStatus_Succeeded
               && MaaTaskerBindResource(tasker_, resource) && MaaTaskerBindController(tasker_, controller_) && MaaTaskerInited(tasker_);
    }

    RecognitionResult run(const json::value& pipeline)
    {
        RecognitionResult result;
        const auto sink = MaaTaskerAddContextSink(tasker_, capture_recognition, &result);
        const auto task_id = MaaTaskerPostTask(tasker_, kEntry, pipeline.to_string().c_str());
        result.task_succeeded = task_id != MaaInvalidId && MaaTaskerWait(tasker_, task_id) == MaaStatus_Succeeded;
        MaaTaskerRemoveContextSink(tasker_, sink);
        return result;
    }

private:
    cv::Mat image_;
    MaaController* controller_ = nullptr;
    MaaTasker* tasker_ = nullptr;
};

json::value ocr_node(const json::value& recognition, bool force_miss)
{
    auto node = recognition;
    node["recognition"] = "OCR";
    node["action"] = "DoNothing";
    node["pre_delay"] = 0;
    node["post_delay"] = 0;
    node["next"] = json::array { };
    if (force_miss) {
        node["expected"] = json::array { "(?!)" };
    }
    return node;
}

json::value make_pipeline(const json::value& recognition, const json::array& peers, std::string_view mode)
{
    json::value pipeline {
        { kTarget, ocr_node(recognition, false) },
        { kFallback, json::object { { "recognition", "DirectHit" }, { "action", "DoNothing" }, { "post_delay", 0 } } },
    };
    json::array next;
    if (mode == "standalone" || mode == "target_first") {
        next.emplace_back(kTarget);
    }
    const auto& selected_peers = mode == "shared_roi_first" ? json::array { recognition } : peers;
    if (mode != "standalone") {
        for (size_t i = 0; i < selected_peers.size(); ++i) {
            auto name = std::format("OCRRegressionPeer{}", i);
            pipeline[name] = ocr_node(selected_peers[i], true);
            next.emplace_back(std::move(name));
        }
    }
    if (mode == "peers_first" || mode == "shared_roi_first") {
        next.emplace_back(kTarget);
    }
    // A failed target must end this round, so failures are reported instead of retried indefinitely.
    next.emplace_back(kFallback);
    pipeline[kEntry] = json::object {
        { "recognition", "DirectHit" },
        { "action", "DoNothing" },
        { "post_delay", 0 },
        { "next", std::move(next) },
    };
    return pipeline;
}

bool valid_case(const json::value& item)
{
    if (!item.is_object() || !item.contains("recognition") || !item.at("recognition").is_object() || !item.contains("peers")
        || !item.at("peers").is_array() || item.at("peers").as_array().empty()) {
        return false;
    }
    const auto& recognition = item.at("recognition");
    if (!recognition.contains("expected")) {
        return false;
    }
    const auto& expected = recognition.at("expected");
    if ((!expected.is_array() || expected.as_array().empty()) && (!expected.is_string() || expected.as_string().empty())) {
        return false;
    }
    for (const auto& peer : item.at("peers").as_array()) {
        if (!peer.is_object()) {
            return false;
        }
    }
    return !item.get("id", std::string()).empty() && !item.get("image", std::string()).empty();
}

json::value run_case(MaaResource* resource, const std::filesystem::path& data_dir, const json::value& item)
{
    json::value report {
        { "id", item.is_object() ? item.get("id", std::string("invalid")) : "invalid" },
        { "passed", false },
    };
    if (!valid_case(item)) {
        report["error"] = "Invalid case: require an image, target expectations and OCR peers";
        return report;
    }
    for (const char* field : { "source",
                               "source_image",
                               "image",
                               "image_sha256",
                               "original_size",
                               "image_size",
                               "recognition",
                               "peers",
                               "derived_from",
                               "notes" }) {
        if (item.contains(field)) {
            report[field] = item.at(field);
        }
    }
    const auto original_size = item.get("original_size", std::vector<int> { });
    const auto image_size = item.get("image_size", std::vector<int> { });
    auto image = MAA_NS::imread(data_dir / item.at("image").as_string());
    if (image.empty() || original_size.size() != 2 || image.size() != cv::Size(original_size[0], original_size[1]) || image_size.size() != 2
        || image_size[0] <= 0 || image_size[1] <= 0) {
        report["error"] = "Image could not be read or its dimensions differ from the manifest";
        return report;
    }
    if (image.size() != cv::Size(image_size[0], image_size[1])) {
        cv::resize(image, image, cv::Size(image_size[0], image_size[1]), 0, 0, cv::INTER_AREA);
    }
    Fixture fixture(std::move(image));
    if (!fixture.init(resource)) {
        report["error"] = "Could not initialize the image controller and tasker";
        return report;
    }

    const auto& recognition = item.at("recognition");
    const auto& peers = item.at("peers").as_array();
    const auto standalone = fixture.run(make_pipeline(recognition, peers, "standalone"));
    report["standalone"] = standalone.to_json();
    bool passed = standalone.succeeded();
    json::array comparisons;
    for (const char* mode : { "target_first", "peers_first", "shared_roi_first" }) {
        const auto batch = fixture.run(make_pipeline(recognition, peers, mode));
        const bool equal = batch.detail == standalone.detail;
        const bool match = standalone.succeeded() && batch.succeeded() && equal;
        comparisons.emplace_back(
            json::object {
                { "mode", mode },
                { "passed", match },
                { "details_equal", equal },
                { "batch", batch.to_json() },
            });
        passed &= match;
        std::cout << "OCRRegression " << item.at("id").as_string() << '/' << mode << ": " << (match ? "PASS" : "FAIL")
                  << "\n  standalone: " << standalone.detail.to_string() << "\n  batch: " << batch.detail.to_string() << '\n';
    }
    report["comparisons"] = std::move(comparisons);
    report["passed"] = passed;
    if (!passed) {
        report["error"] = standalone.succeeded() ? "Batch recognition missed or differs from standalone all/filtered/best"
                                                 : "Standalone OCR did not match the expected text";
    }
    return report;
}

json::value run_four_roi_regression(MaaResource* resource, const std::filesystem::path& data_dir, const json::array& corpus_cases)
{
    constexpr const char* id = "1448-hbr-august-four-independent-rois";
    constexpr const char* source_id = "1448-hbr-august-fullscreen";
    const json::value* source = nullptr;
    for (const auto& item : corpus_cases) {
        if (item.is_object() && item.get("id", std::string()) == source_id) {
            source = &item;
            break;
        }
    }

    const json::value target_roi = json::array { 1102, 186, 178, 150 };
    if (!source || !valid_case(*source) || !source->at("recognition").contains("roi")
        || source->at("recognition").at("roi") != target_roi) {
        return json::object {
            { "id", id },
            { "derived_from", source_id },
            { "passed", false },
            { "error", "Missing or changed source case for the four-independent-ROI regression" },
        };
    }

    // Keep the published issue corpus unchanged. These additional crop positions
    // exercise a genuine four-image tensor batch on the original screenshot.
    auto item = *source;
    item["id"] = id;
    item["derived_from"] = source_id;
    item["peers"] = json::array {
        json::object { { "roi", json::array { 900, 186, 178, 150 } } },
        json::object { { "roi", json::array { 700, 186, 178, 150 } } },
        json::object { { "roi", json::array { 1102, 400, 178, 150 } } },
    };
    item["notes"] =
        "Framework-derived four-ROI fixture. The original issue image and target recognition are unchanged; "
        "the three independent peer crops are test parameters, not settings reported in the issue. "
        "target_first and peers_first each supply four distinct 178x150 ROIs (detector shape 192x160). "
        "shared_roi_first remains a duplicate-ROI cache control. Compare all/filtered/best exactly.";
    return run_case(resource, data_dir, item);
}

bool save_report(const std::filesystem::path& report_dir, const json::value& report)
{
    std::error_code error;
    std::filesystem::create_directories(report_dir, error);
    if (error) {
        std::cerr << "OCRRegression: cannot create report directory: " << error.message() << '\n';
        return false;
    }
    std::ofstream output(report_dir / "ocr-regression.json", std::ios::binary);
    output << report.to_string();
    return static_cast<bool>(output);
}

} // namespace

bool ocr_regression(const std::filesystem::path& testset_dir, const std::filesystem::path& report_dir)
{
    const auto data_dir = testset_dir / "OCRRegression";
    const auto manifest = json::open(data_dir / "cases.json");
    json::value report {
        { "version", 1 },    { "framework_version", MaaVersion() }, { "provider", "CPU" }, { "resize_method", "INTER_AREA" },
        { "passed", false },
    };
    if (!manifest || manifest->get("version", 0) != 1 || !manifest->contains("cases") || !manifest->at("cases").is_array()
        || manifest->at("cases").as_array().empty()) {
        report["error"] = "Missing or invalid OCRRegression/cases.json";
        save_report(report_dir, report);
        std::cerr << "OCRRegression: " << report.at("error").as_string() << '\n';
        return false;
    }

    std::unique_ptr<MaaResource, decltype(&MaaResourceDestroy)> resource(MaaResourceCreate(), MaaResourceDestroy);
    MaaInferenceExecutionProvider provider = MaaInferenceExecutionProvider_CPU;
    MaaInferenceDevice device = MaaInferenceDevice_CPU;
    if (!resource || !MaaResourceSetOption(resource.get(), MaaResOption_InferenceExecutionProvider, &provider, sizeof(provider))
        || !MaaResourceSetOption(resource.get(), MaaResOption_InferenceDevice, &device, sizeof(device))
        || MaaResourceWait(resource.get(), MaaResourcePostBundle(resource.get(), (data_dir / "resource").string().c_str()))
               != MaaStatus_Succeeded) {
        report["error"] = "Could not load OCRRegression/resource with CPU inference";
        save_report(report_dir, report);
        std::cerr << "OCRRegression: " << report.at("error").as_string() << '\n';
        return false;
    }

    size_t failed = 0;
    json::array cases;
    for (const auto& item : manifest->at("cases").as_array()) {
        auto result = run_case(resource.get(), data_dir, item);
        if (!result.at("passed").as_boolean()) {
            ++failed;
            std::cerr << "OCRRegression " << result.at("id").as_string() << ": FAIL: " << result.at("error").as_string() << '\n';
        }
        cases.emplace_back(std::move(result));
    }
    report["total"] = cases.size();
    report["failed"] = failed;
    report["cases"] = std::move(cases);

    auto four_roi = run_four_roi_regression(resource.get(), data_dir, manifest->at("cases").as_array());
    const bool supplemental_passed = four_roi.at("passed").as_boolean();
    if (!supplemental_passed) {
        std::cerr << "OCRRegression " << four_roi.at("id").as_string() << ": FAIL: " << four_roi.at("error").as_string() << '\n';
    }
    report["supplemental_total"] = 1;
    report["supplemental_failed"] = supplemental_passed ? 0 : 1;
    report["supplemental_cases"] = json::array { std::move(four_roi) };
    report["passed"] = failed == 0 && supplemental_passed;

    const bool saved = save_report(report_dir, report);
    std::cout << "OCRRegression: " << report.at("total").as_integer() << " corpus cases, " << failed << " failed; "
              << "1 supplemental case, " << (supplemental_passed ? 0 : 1) << " failed; report: " << (report_dir / "ocr-regression.json")
              << '\n';
    return failed == 0 && supplemental_passed && saved;
}
