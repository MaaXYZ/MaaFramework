#include "OCRCondition.h"

#include <charconv>
#include <cmath>
#include <locale>
#include <sstream>
#include <unordered_map>

#include "MaaUtils/Encoding.h"

MAA_VISION_NS_BEGIN

namespace
{
constexpr char kNumberPattern[] = R"([+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?)";

std::optional<double> parse_number(std::string_view text)
{
    static const boost::regex grammar(kNumberPattern);
    if (!boost::regex_match(text.begin(), text.end(), grammar)) {
        return std::nullopt;
    }
    // Use a fixed decimal separator, including on platforms without floating-point from_chars.
    std::istringstream stream { std::string(text) };
    stream.imbue(std::locale::classic());
    double value = 0;
    if (!(stream >> std::noskipws >> value) || !stream.eof() || !std::isfinite(value)) {
        return std::nullopt;
    }
    // Some standard libraries silently round an underflowing nonzero value to zero.
    if (value == 0 && text.substr(0, text.find_first_of("eE")).find_first_of("123456789") != std::string_view::npos) {
        return std::nullopt;
    }
    return value;
}
}

std::optional<OCRCondition> OCRCondition::parse(const std::string& expression)
{
    OCRCondition result;
    if (expression.empty()) {
        return result;
    }

    static const std::string operand = std::string(R"(\$[0-9]+|)") + kNumberPattern;
    static const boost::regex grammar("\\s*(" + operand + ")\\s*(>=|<=|==|!=|>|<)\\s*(" + operand + ")\\s*");
    static const std::unordered_map<std::string, Operator> operators = {
        { ">", Operator::Greater },    { ">=", Operator::GreaterEqual }, { "<", Operator::Less },
        { "<=", Operator::LessEqual }, { "==", Operator::Equal },        { "!=", Operator::NotEqual },
    };

    boost::smatch match;
    if (!boost::regex_match(expression, match, grammar) || !parse_operand(match[1].str(), result.lhs_)
        || !parse_operand(match[3].str(), result.rhs_)) {
        return std::nullopt;
    }

    result.operator_ = operators.at(match[2].str());
    result.expression_ = expression;
    return result;
}

bool OCRCondition::parse_operand(std::string_view text, Operand& output)
{
    if (text.front() != '$') {
        auto value = parse_number(text);
        if (!value) {
            return false;
        }
        output = *value;
        return true;
    }

    size_t index = 0;
    const auto [end, error] = std::from_chars(text.data() + 1, text.data() + text.size(), index);
    if (error != std::errc() || end != text.data() + text.size()) {
        return false;
    }
    output = index;
    return true;
}

std::optional<double> OCRCondition::resolve(const Operand& operand, std::wstring_view text, const boost::wsmatch& match)
{
    if (const auto* value = std::get_if<double>(&operand)) {
        return *value;
    }

    const auto index = std::get<size_t>(operand);
    if (!match.empty()) {
        if (index >= match.size() || !match[index].matched) {
            return std::nullopt;
        }
        text = std::wstring_view(match[index].first, match[index].second);
    }
    else if (index != 0) {
        return std::nullopt;
    }

    return parse_number(from_u16(std::wstring(text)));
}

bool OCRCondition::evaluate(std::wstring_view text, const boost::wsmatch& match) const
{
    if (expression_.empty()) {
        return true;
    }

    const auto lhs = resolve(lhs_, text, match);
    const auto rhs = resolve(rhs_, text, match);
    if (!lhs || !rhs) {
        return false;
    }

    switch (operator_) {
    case Operator::Greater:
        return *lhs > *rhs;
    case Operator::GreaterEqual:
        return *lhs >= *rhs;
    case Operator::Less:
        return *lhs < *rhs;
    case Operator::LessEqual:
        return *lhs <= *rhs;
    case Operator::Equal:
        return *lhs == *rhs;
    case Operator::NotEqual:
        return *lhs != *rhs;
    }
    return false;
}

MAA_VISION_NS_END
