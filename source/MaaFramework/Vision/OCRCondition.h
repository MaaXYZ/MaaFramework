#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include <boost/regex.hpp>

#include "Common/Conf.h"

MAA_VISION_NS_BEGIN

class OCRCondition
{
public:
    static std::optional<OCRCondition> parse(const std::string& expression);

    const std::string& expression() const { return expression_; }

    bool evaluate(std::wstring_view text, const boost::wsmatch& match = boost::wsmatch()) const;

private:
    enum class Operator
    {
        Greater,
        GreaterEqual,
        Less,
        LessEqual,
        Equal,
        NotEqual,
    };

    using Operand = std::variant<double, size_t>;

    static bool parse_operand(std::string_view text, Operand& output);
    static std::optional<double> resolve(const Operand& operand, std::wstring_view text, const boost::wsmatch& match);

    std::string expression_;
    Operand lhs_ = 0.0;
    Operand rhs_ = 0.0;
    Operator operator_ = Operator::Equal;
};

MAA_VISION_NS_END
