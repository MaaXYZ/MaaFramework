#include "OCRCondition.h"

#include <iostream>
#include <locale>

#include "MaaUtils/Encoding.h"
#include "Vision/OCRCondition.h"
#include "Vision/OCRer.h"

namespace
{
using namespace MAA_VISION_NS;

class CacheOnlyOCRSession : public OCRSession
{
public:
    std::optional<OCRDetRecResult> det_and_rec(const cv::Mat&) override { return std::nullopt; }

    std::optional<OCRRecResult> rec(const cv::Mat&) override { return std::nullopt; }

    std::optional<std::vector<OCRRecResult>> batch_rec(const std::vector<cv::Mat>&) override { return std::nullopt; }
};

bool check(bool value, const std::string& message)
{
    if (!value) {
        std::cerr << "OCRCondition: " << message << '\n';
    }
    return value;
}

bool test_comparisons()
{
    struct Case
    {
        std::string condition;
        std::wstring text;
        std::wstring expected;
        bool hit;
    };

    const Case cases[] = {
        { "$2 > 50", L"30/100", LR"((\d+)/(\d+))", true },
        { "$2 > 100", L"30/100", LR"((\d+)/(\d+))", false },
        { "$2 >= 100", L"30/100", LR"((\d+)/(\d+))", true },
        { "$2 >= 101", L"30/100", LR"((\d+)/(\d+))", false },
        { "$1 < $2", L"30/100", LR"((\d+)/(\d+))", true },
        { "$2 < $1", L"30/100", LR"((\d+)/(\d+))", false },
        { "$1 <= $2", L"100/100", LR"((\d+)/(\d+))", true },
        { "$1 <= $2", L"101/100", LR"((\d+)/(\d+))", false },
        { "$1 == $2", L"100/100", LR"((\d+)/(\d+))", true },
        { "$1 == $2", L"30/100", LR"((\d+)/(\d+))", false },
        { "$1 != $2", L"30/100", LR"((\d+)/(\d+))", true },
        { "$1 != $2", L"100/100", LR"((\d+)/(\d+))", false },
        { "50 < $0", L"remaining:100", LR"(\d+)", true },
        { "$1 == -1.25", L"-1.25", L"(.*)", true },
        { "$1 == +.5", L"+0.5", L"(.*)", true },
        { "$1 == 1e2", L"100", L"(.*)", true },
        { "$1 == 100", L"+1e2", L"(.*)", true },
        { "\t$1>=10.\n", L"10", L"(.*)", true },
        { "1 < 2", L"text", L"text", true },
        { "", L"text", L"text", true },
        { "$3 > 0", L"30/100", LR"((\d+)/(\d+))", false },
        { "$1 != 0", L"text", LR"((\d+)?text)", false },
        { "$1 != 0", L"", L"(.*)", false },
        { "$1 != 0", L"100abc", L"(.*)", false },
        { "$1 != 0", L"NaN", L"(.*)", false },
        { "$1 != 0", L"inf", L"(.*)", false },
        { "$1 != 0", L"1e9999", L"(.*)", false },
        { "$1 != 0", L"1e-9999", L"(.*)", false },
        { "$1 != 0", L"0x10", L"(.*)", false },
        { "$1 != 0", L"+-1", L"(.*)", false },
        { "$1 != 0", L"１２", L"(.*)", false },
    };
    bool passed = true;
    for (const auto& test : cases) {
        auto condition = OCRCondition::parse(test.condition);
        boost::wsmatch match;
        const bool matched = boost::regex_search(test.text, match, boost::wregex(test.expected));
        passed &= check(
            condition && matched && condition->evaluate(test.text, match) == test.hit,
            test.condition + " (text: " + MAA_NS::from_u16(test.text) + ")");
    }
    return passed;
}

bool test_filtering()
{
    const cv::Mat image(64, 256, CV_8UC3, cv::Scalar(255, 255, 255));
    const std::vector<cv::Rect> rois { { 0, 0, image.cols, image.rows } };
    const OCRer::ResultsVec cached {
        { L" 30/1OO ", { 40, 10, 20, 10 }, 0.9 },
        { L"20/40", { 10, 10, 20, 10 }, 0.9 },
        { L"60/200", { 70, 10, 20, 10 }, 0.9 },
        { L"40/300", { 100, 10, 20, 10 }, 0.1 },
    };
    auto session = std::make_shared<CacheOnlyOCRSession>();
    OCRCache cache;
    cache.insert(session, rois.front(), cached);
    OCRerParam param;
    param.expected = { LR"((\d+)/(\d+))" };
    param.replace = { { L"O", L"0" } };
    param.condition = *OCRCondition::parse("$2 > 50");
    param.result_index = 1;
    OCRer ocr(image, rois, param, session, "", std::nullopt, &cache);
    bool passed = check(ocr.filtered_results().size() == 2, "filter after replace and confidence, before index");
    passed &= check(ocr.best_result() && ocr.best_result()->text == L"60/200", "index counts only passing results");
    passed &= check(ocr.all_results().size() == 4, "preserve unfiltered results");

    param.result_index = 0;
    OCRer first(image, rois, param, session, "", std::nullopt, &cache);
    passed &= check(
        first.best_result() && first.best_result()->text == L"30/100" && first.best_result()->box == cached[0].box,
        "preserve replaced text and original box");

    param.condition = { };
    OCRer legacy(image, rois, param, session, "", std::nullopt, &cache);
    passed &=
        check(legacy.filtered_results().size() == 3 && legacy.best_result()->text == L"20/40", "empty condition preserves old behavior");

    param.condition = *OCRCondition::parse("$1 > 50");
    param.expected = { LR"((\d+)/\d+)", LR"(\d+/(\d+))" };
    OCRer alternatives(image, rois, param, session, "", std::nullopt, &cache);
    passed &= check(alternatives.filtered_results().size() == 2, "try remaining expected patterns when a condition fails");

    const OCRer::ResultsVec numbers { { L" 100 ", { 10, 10, 20, 10 }, 0.9 } };
    cache.insert(session, rois.front(), numbers);
    param.expected.clear();
    param.condition = *OCRCondition::parse("$0 > 50");
    OCRer whole_text(image, rois, param, session, "", std::nullopt, &cache);
    passed &= check(whole_text.best_result().has_value(), "$0 refers to full text without expected");
    param.condition = *OCRCondition::parse("$1 != 0");
    OCRer missing_group(image, rois, param, session, "", std::nullopt, &cache);
    passed &= check(!missing_group.best_result(), "no capture groups without expected");

    param.expected = { LR"((\d+))" };
    param.condition = *OCRCondition::parse("$1 > 50");
    const OCRer::ResultsVec repeated { { L"20 100", { 10, 10, 20, 10 }, 0.9 } };
    cache.insert(session, rois.front(), repeated);
    OCRer first_match(image, rois, param, session, "", std::nullopt, &cache);
    passed &= check(!first_match.best_result(), "use the first match of each regex");
    return passed;
}

bool test_decimal_locale()
{
    class CommaPunctuation : public std::numpunct<char>
    {
        char do_decimal_point() const override { return ','; }
    };

    const auto previous = std::locale::global(std::locale(std::locale::classic(), new CommaPunctuation));
    const auto condition = OCRCondition::parse("$0 == 1.5");
    const bool passed = condition && condition->evaluate(L"1.5") && !condition->evaluate(L"1,5");
    std::locale::global(previous);
    return check(passed, "numeric parsing must not depend on the process locale");
}
}

bool test_ocr_condition()
{
    const bool passed = test_comparisons() && test_filtering() && test_decimal_locale();
    if (passed) {
        std::cout << "OCRCondition: comparisons, captures, conversion failures and OCR filtering passed\n";
    }
    return passed;
}
