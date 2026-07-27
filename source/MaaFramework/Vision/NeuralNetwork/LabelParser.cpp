#include "LabelParser.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <format>
#include <unordered_set>

MAA_VISION_NS_BEGIN

namespace NeuralNetwork
{

namespace
{

void skip_spaces(std::string_view text, size_t& position)
{
    while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position]))) {
        ++position;
    }
}

bool parse_python_dict(std::string_view text, std::vector<std::string>& labels, std::string& error)
{
    size_t position = 0;
    skip_spaces(text, position);
    if (position == text.size() || text[position++] != '{') {
        error = "metadata labels are not a supported JSON value or Python dictionary";
        return false;
    }

    std::vector<std::pair<size_t, std::string>> entries;
    for (;;) {
        skip_spaces(text, position);
        if (position < text.size() && text[position] == '}') {
            ++position;
            break;
        }

        const size_t key_begin = position;
        while (position < text.size() && std::isdigit(static_cast<unsigned char>(text[position]))) {
            ++position;
        }
        size_t key = 0;
        const auto [key_end, key_error] = std::from_chars(text.data() + key_begin, text.data() + position, key);
        if (key_begin == position || key_error != std::errc() || key_end != text.data() + position) {
            error = "Python metadata label keys must be non-negative decimal class ids";
            return false;
        }

        skip_spaces(text, position);
        if (position == text.size() || text[position++] != ':') {
            error = "invalid Python metadata labels dictionary";
            return false;
        }
        skip_spaces(text, position);
        if (position == text.size() || (text[position] != '\'' && text[position] != '"')) {
            error = "Python metadata labels must be strings";
            return false;
        }

        const char quote = text[position++];
        std::string label;
        bool closed = false;
        while (position < text.size()) {
            const char current = text[position++];
            if (current == quote) {
                closed = true;
                break;
            }
            if (current == '\\') {
                if (position == text.size()) {
                    error = "invalid escape in Python metadata label";
                    return false;
                }
                label.push_back(text[position++]);
                continue;
            }
            label.push_back(current);
        }
        if (!closed) {
            error = "unterminated Python metadata label";
            return false;
        }
        entries.emplace_back(key, std::move(label));

        skip_spaces(text, position);
        if (position < text.size() && text[position] == ',') {
            ++position;
            continue;
        }
        if (position < text.size() && text[position] == '}') {
            ++position;
            break;
        }
        error = "invalid Python metadata labels dictionary";
        return false;
    }

    skip_spaces(text, position);
    if (position != text.size()) {
        error = "unexpected characters after Python metadata labels dictionary";
        return false;
    }

    std::ranges::sort(entries, { }, &std::pair<size_t, std::string>::first);
    labels.clear();
    labels.reserve(entries.size());
    for (size_t index = 0; index < entries.size(); ++index) {
        if (entries[index].first != index) {
            error = "metadata label class ids must be contiguous from zero";
            return false;
        }
        labels.emplace_back(std::move(entries[index].second));
    }
    return true;
}

} // namespace

bool LabelParser::parse_metadata(std::string_view text, std::vector<std::string>& labels, std::string& error)
{
    if (auto value = json::parse(text)) {
        return parse(*value, labels, error);
    }

    std::vector<std::string> parsed;
    if (!parse_python_dict(text, parsed, error)) {
        return false;
    }
    return validate(std::move(parsed), labels, error);
}

bool LabelParser::parse(const json::value& value, std::vector<std::string>& labels, std::string& error)
{
    std::vector<std::string> parsed;
    if (value.is_array()) {
        for (const auto& item : value.as_array()) {
            if (!item.is_string()) {
                error = "labels array must contain only strings";
                return false;
            }
            parsed.emplace_back(item.as_string());
        }
    }
    else if (value.is_object()) {
        std::vector<std::pair<size_t, std::string>> entries;
        entries.reserve(value.as_object().size());
        for (const auto& [key_text, item] : value.as_object()) {
            size_t key = 0;
            const auto [end, conversion_error] = std::from_chars(key_text.data(), key_text.data() + key_text.size(), key);
            if (conversion_error != std::errc() || end != key_text.data() + key_text.size() || !item.is_string()) {
                error = "labels object must map decimal class ids to strings";
                return false;
            }
            entries.emplace_back(key, item.as_string());
        }
        std::ranges::sort(entries, { }, &std::pair<size_t, std::string>::first);
        for (size_t index = 0; index < entries.size(); ++index) {
            if (entries[index].first != index) {
                error = "label class ids must be contiguous from zero";
                return false;
            }
            parsed.emplace_back(std::move(entries[index].second));
        }
    }
    else {
        error = "labels must be an array or object";
        return false;
    }

    return validate(std::move(parsed), labels, error);
}

bool LabelParser::validate(std::vector<std::string> labels, std::vector<std::string>& output, std::string& error)
{
    if (labels.empty()) {
        error = "labels must not be empty";
        return false;
    }

    std::unordered_set<std::string> unique;
    unique.reserve(labels.size());
    for (const auto& label : labels) {
        if (label.empty()) {
            error = "labels must not contain empty strings";
            return false;
        }
        if (!unique.emplace(label).second) {
            error = "labels must be unique";
            return false;
        }
    }

    output = std::move(labels);
    error.clear();
    return true;
}

bool LabelParser::resolve(
    const std::vector<std::string>& pipeline,
    const std::vector<std::string>& descriptor,
    const std::vector<std::string>& metadata,
    std::vector<std::string>& output,
    std::string& error)
{
    if (!pipeline.empty()) {
        return validate(pipeline, output, error);
    }
    if (!descriptor.empty()) {
        return validate(descriptor, output, error);
    }
    if (!metadata.empty()) {
        return validate(metadata, output, error);
    }

    output.clear();
    error.clear();
    return true;
}

bool LabelParser::label_for(int class_id, const std::vector<std::string>& labels, std::string& output, std::string& error)
{
    if (class_id < 0) {
        error = "class id must be non-negative";
        return false;
    }
    if (labels.empty()) {
        output = std::format("Unknown_{}", class_id);
        error.clear();
        return true;
    }
    if (static_cast<size_t>(class_id) >= labels.size()) {
        error = "class id is outside the available label table";
        return false;
    }

    output = labels[static_cast<size_t>(class_id)];
    error.clear();
    return true;
}

} // namespace NeuralNetwork

MAA_VISION_NS_END
