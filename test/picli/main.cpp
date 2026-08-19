#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>
#include <utility>

#include "CLI/interactor.h"
#include "ProjectInterface/Parser.h"

using namespace MAA_PROJECT_INTERFACE_NS;

namespace
{
std::optional<InterfaceData> parse_interface(std::string_view welcome)
{
    const std::string source = std::string(R"({
        "interface_version": 2,
        "controller": [{ "name": "Default", "type": "Adb" }],
        "resource": [{ "name": "Default", "path": ["resource"] }],
        "welcome": )") + std::string(welcome)
                               + "}";

    auto json_opt = json::parse(source);
    if (!json_opt) {
        return std::nullopt;
    }
    return Parser::parse_interface(*json_opt);
}

bool check(bool condition, std::string_view message)
{
    if (condition) {
        return true;
    }

    std::cerr << message << '\n';
    return false;
}

bool test_legacy_welcome()
{
    auto data_opt = parse_interface(R"("Welcome")");
    if (!check(data_opt.has_value(), "legacy welcome should parse")) {
        return false;
    }

    const auto* welcome = std::get_if<std::string>(&data_opt->welcome);
    return check(welcome && *welcome == "Welcome", "legacy welcome should remain a string");
}

bool test_welcome_items()
{
    auto data_opt = parse_interface(R"([
        { "label": "$title", "content": "$content" },
        { "content": "notice.md" }
    ])");
    if (!check(data_opt.has_value(), "welcome item array should parse")) {
        return false;
    }

    const auto* items = std::get_if<std::vector<InterfaceData::WelcomeItem>>(&data_opt->welcome);
    return check(items && items->size() == 2, "welcome item order should be preserved")
           && check((*items)[0].label == "$title" && (*items)[0].content == "$content", "i18n fields should be preserved")
           && check((*items)[1].label.empty() && (*items)[1].content == "notice.md", "label should be optional");
}

bool test_invalid_welcome_items()
{
    return check(!parse_interface("[]"), "empty welcome array should be rejected")
           && check(!parse_interface(R"([{ "label": "Missing content" }])"), "welcome content should be required")
           && check(
               !parse_interface(R"([{ "content": "Notice", "unexpected": true }])"),
               "welcome item properties not declared by the schema should be rejected");
}

bool test_welcome_tracking()
{
    Configuration config;
    InterfaceData::Welcome welcome = std::vector<InterfaceData::WelcomeItem> {
        { "First", "first.md" },
        { "Second", "second.md" },
    };
    InterfaceData::Welcome resolved_welcome = std::vector<InterfaceData::WelcomeItem> {
        { "First", "First content" },
        { "Second", "Second content" },
    };

    if (!check(config.has_welcome_update(welcome, resolved_welcome), "unseen welcome should be reported as updated")) {
        return false;
    }
    config.acknowledge_welcome_update(welcome, resolved_welcome);
    if (!check(!config.has_welcome_update(welcome, resolved_welcome), "acknowledged welcome should not be repeated")) {
        return false;
    }

    auto reordered = welcome;
    auto& reordered_items = std::get<std::vector<InterfaceData::WelcomeItem>>(reordered);
    std::swap(reordered_items[0], reordered_items[1]);
    if (!check(config.has_welcome_update(reordered, resolved_welcome), "reordered welcome should be reported as updated")) {
        return false;
    }

    auto added = welcome;
    std::get<std::vector<InterfaceData::WelcomeItem>>(added).push_back({ "Third", "third.md" });
    if (!check(config.has_welcome_update(added, resolved_welcome), "added welcome item should be reported as updated")) {
        return false;
    }

    auto removed = welcome;
    std::get<std::vector<InterfaceData::WelcomeItem>>(removed).pop_back();
    if (!check(config.has_welcome_update(removed, resolved_welcome), "removed welcome item should be reported as updated")) {
        return false;
    }

    auto changed_label = welcome;
    std::get<std::vector<InterfaceData::WelcomeItem>>(changed_label)[0].label = "Changed";
    if (!check(config.has_welcome_update(changed_label, resolved_welcome), "changed label should be reported as updated")) {
        return false;
    }

    auto changed_content = welcome;
    std::get<std::vector<InterfaceData::WelcomeItem>>(changed_content)[0].content = "changed.md";
    if (!check(config.has_welcome_update(changed_content, resolved_welcome), "changed content should be reported as updated")) {
        return false;
    }

    auto changed_translated_label = resolved_welcome;
    std::get<std::vector<InterfaceData::WelcomeItem>>(changed_translated_label)[0].label = "Translated title changed";
    if (!check(config.has_welcome_update(welcome, changed_translated_label), "changed translated label should be reported as updated")) {
        return false;
    }

    auto changed_file_content = resolved_welcome;
    std::get<std::vector<InterfaceData::WelcomeItem>>(changed_file_content)[0].content = "Updated file content";
    if (!check(config.has_welcome_update(welcome, changed_file_content), "changed file content should be reported as updated")) {
        return false;
    }

    InterfaceData::Welcome empty_welcome = std::string();
    if (!check(config.has_welcome_update(empty_welcome, empty_welcome), "removed welcome should be reported as updated")) {
        return false;
    }
    config.acknowledge_welcome_update(empty_welcome, empty_welcome);
    return check(config.has_welcome_update(welcome, resolved_welcome), "welcome restored after removal should be reported as updated");
}

bool test_welcome_tracking_round_trip()
{
    Configuration config;
    config.controller.name = "Default";
    config.resource = "Default";
    InterfaceData::Welcome welcome = std::vector<InterfaceData::WelcomeItem> {
        { "$title", "notice.md" },
    };
    InterfaceData::Welcome resolved_welcome = std::vector<InterfaceData::WelcomeItem> {
        { "Title", "Resolved notice" },
    };
    config.acknowledge_welcome_update(welcome, resolved_welcome);

    auto config_json = config.to_json();
    if (!check(
            config_json.exists("last_welcome") && config_json.exists("last_resolved_welcome"),
            "serialized configuration should preserve both welcome snapshots")) {
        return false;
    }

    auto parsed_opt = Parser::parse_config(config_json);
    if (!check(parsed_opt.has_value(), "configuration with welcome state should parse")
        || !check(!parsed_opt->has_welcome_update(welcome, resolved_welcome), "welcome state should survive configuration serialization")) {
        return false;
    }

    config_json.as_object().erase("last_resolved_welcome");
    auto legacy_config_opt = Parser::parse_config(config_json);
    return check(legacy_config_opt.has_value(), "configuration with only the legacy welcome snapshot should parse")
           && check(
               legacy_config_opt->has_welcome_update(welcome, resolved_welcome),
               "legacy welcome state should be refreshed with resolved content");
}

std::optional<std::filesystem::path> create_temp_directory()
{
    std::error_code ec;
    const auto base = std::filesystem::temp_directory_path(ec);
    if (ec) {
        return std::nullopt;
    }

    for (size_t index = 0; index < 1000; ++index) {
        auto path = base / ("maafw-picli-testing-" + std::to_string(index));
        if (std::filesystem::create_directory(path, ec)) {
            return path;
        }
        if (ec) {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

class TempDirectoryGuard
{
public:
    explicit TempDirectoryGuard(std::filesystem::path path)
        : path_(std::move(path))
    {
    }

    ~TempDirectoryGuard()
    {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

private:
    std::filesystem::path path_;
};

bool write_text_file(const std::filesystem::path& path, std::string_view content)
{
    std::error_code ec;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) {
            return false;
        }
    }

    std::ofstream output(path, std::ios::binary);
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    return output.good();
}

std::optional<std::string> run_interaction(const std::filesystem::path& project_path)
{
    Interactor interactor(project_path);
    if (!interactor.load(project_path)) {
        return std::nullopt;
    }

    std::istringstream input("7\n");
    std::ostringstream output;
    auto* original_input = std::cin.rdbuf(input.rdbuf());
    auto* original_output = std::cout.rdbuf(output.rdbuf());
    interactor.interact();
    std::cin.rdbuf(original_input);
    std::cout.rdbuf(original_output);
    return output.str();
}

bool test_interactor_welcome_lifecycle()
{
    auto project_path_opt = create_temp_directory();
    if (!check(project_path_opt.has_value(), "temporary project directory should be created")) {
        return false;
    }
    const auto& project_path = *project_path_opt;
    TempDirectoryGuard guard(project_path);

    constexpr std::string_view interface_json = R"({
        "interface_version": 2,
        "name": "WelcomeTest",
        "languages": {
            "c": "translations.json",
            "en": "translations.json",
            "en_us": "translations.json",
            "zh": "translations.json",
            "zh_cn": "translations.json"
        },
        "controller": [{ "name": "Default", "type": "Adb" }],
        "resource": [{ "name": "Default", "path": ["resource"] }],
        "welcome": [
            { "label": "$first_label", "content": "$first_content" },
            { "content": "$second_content" }
        ]
    })";
    constexpr std::string_view translations_json = R"({
        "first_label": "First announcement",
        "first_content": "first.md",
        "second_content": "Second announcement content"
    })";

    Configuration config;
    config.controller.name = "Default";
    config.resource = "Default";
    auto config_json = config.to_json();
    config_json.as_object().erase("last_welcome");
    config_json.as_object().erase("last_resolved_welcome");

    if (!check(write_text_file(project_path / "interface.json", interface_json), "test interface should be written")
        || !check(write_text_file(project_path / "translations.json", translations_json), "test translations should be written")
        || !check(write_text_file(project_path / "first.md", "First announcement content"), "test announcement should be written")
        || !check(
            write_text_file(project_path / "config/maa_pi_config.json", config_json.dumps(4)),
            "test configuration should be written")) {
        return false;
    }
    if (!check(
            Parser::parse_config(project_path / "config/maa_pi_config.json").has_value(),
            "test configuration should load before interaction")) {
        return false;
    }

    auto first_output = run_interaction(project_path);
    if (!check(first_output.has_value(), "first interaction should complete")) {
        return false;
    }
    const auto first_label = first_output->find("First announcement");
    const auto first_content = first_output->find("First announcement content");
    const auto second_content = first_output->find("Second announcement content");
    if (!check(
            first_label != std::string::npos && first_content != std::string::npos && second_content != std::string::npos
                && first_label < first_content && first_content < second_content,
            "resolved welcome items should be displayed in declaration order")) {
        return false;
    }

    auto saved_config = Parser::parse_config(project_path / "config/maa_pi_config.json");
    if (!check(saved_config.has_value(), "acknowledged welcome state should be persisted")) {
        return false;
    }
    InterfaceData::Welcome raw_welcome = std::vector<InterfaceData::WelcomeItem> {
        { "$first_label", "$first_content" },
        { "", "$second_content" },
    };
    InterfaceData::Welcome resolved_welcome = std::vector<InterfaceData::WelcomeItem> {
        { "First announcement", "First announcement content" },
        { "", "Second announcement content" },
    };
    if (!check(
            !saved_config->has_welcome_update(raw_welcome, resolved_welcome),
            "persisted welcome state should contain raw and resolved snapshots")) {
        return false;
    }

    auto repeated_output = run_interaction(project_path);
    if (!check(repeated_output.has_value(), "repeated interaction should complete")
        || !check(
            repeated_output->find("First announcement content") == std::string::npos
                && repeated_output->find("Second announcement content") == std::string::npos,
            "acknowledged welcome items should not be displayed again")) {
        return false;
    }

    if (!check(
            write_text_file(project_path / "first.md", "Updated first announcement content"),
            "updated announcement should be written")) {
        return false;
    }
    auto updated_output = run_interaction(project_path);
    if (!check(updated_output.has_value(), "interaction after a content update should complete")) {
        return false;
    }
    const auto updated_content = updated_output->find("Updated first announcement content");
    const auto repeated_second_content = updated_output->find("Second announcement content");
    return check(
        updated_content != std::string::npos && repeated_second_content != std::string::npos && updated_content < repeated_second_content,
        "resolved file content changes should redisplay the complete ordered welcome list");
}
} // namespace

int main()
{
    const bool passed = test_legacy_welcome() && test_welcome_items() && test_invalid_welcome_items() && test_welcome_tracking()
                        && test_welcome_tracking_round_trip() && test_interactor_welcome_lifecycle();
    return passed ? 0 : 1;
}
