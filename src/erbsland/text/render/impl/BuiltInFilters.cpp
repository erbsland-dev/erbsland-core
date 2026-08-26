// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BuiltInFilters.hpp"

#include "ValueComparison.hpp"

#include "../Value.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../math/SaturatingMath.hpp"
#include "../../../text/CharSet.hpp"
#include "../../../text/json/JsonContainers.hpp"
#include "../../../text/json/JsonFormatOptions.hpp"
#include "../../../text/json/JsonValue.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringList.hpp"
#include "../../../text/StringSide.hpp"

#include <cmath>

namespace erbsland::text::render::impl::built_in_filters {

using namespace text::literals;

void requireArity(const ValueList &arguments, const std::size_t minimum, const std::size_t maximum) {
    const auto count = arguments.count().toSizeT();
    if (count < minimum || count > maximum) {
        throw err::ParameterError{"The built-in filter received an invalid number of arguments."_el, "arguments"_el};
    }
}

auto argument(const ValueList &arguments, const std::size_t index) -> const Value & {
    return arguments.getRefOrThrow(unit::ItemIndex::fromSizeT(index));
}

auto list(const Value &value) -> const ValueList & {
    if (value.isList()) {
        return value.asList();
    }
    throw err::ParameterError{"The built-in filter requires a list value."_el, "value"_el};
}

auto map(const Value &value) -> const ValueMap & {
    if (value.isMap()) {
        return value.asMap();
    }
    throw err::ParameterError{"The built-in filter requires a map value."_el, "value"_el};
}

auto scalarText(const Value &value) -> String {
    if (value.isList() || value.isMap() || value.isCallback()) {
        throw err::ParameterError{"The built-in filter requires a scalar value."_el, "value"_el};
    }
    return value.toString();
}

auto less(const Value &left, const Value &right, const bool caseSensitive) -> bool {
    if ((left.isInteger() || left.isFloat()) && (right.isInteger() || right.isFloat())) {
        return value_comparison::compareNumbers(left, right) < 0;
    }
    if (left.isText() && right.isText()) {
        if (caseSensitive) {
            return left.asText() < right.asText();
        }
        return left.asText().transformed(Char::toLowercase) < right.asText().transformed(Char::toLowercase);
    }
    throw err::ParameterError{"The values cannot be ordered by this built-in filter."_el, "value"_el};
}

auto toJson(const Value &value) -> json::JsonValue {
    if (value.isNull()) {
        return {};
    }
    if (value.isBoolean()) {
        return json::JsonValue{value.asBoolean()};
    }
    if (value.isInteger()) {
        return json::JsonValue{value.asInteger()};
    }
    if (value.isFloat()) {
        return json::JsonValue{value.asFloat()};
    }
    if (value.isText()) {
        return json::JsonValue{value.asText()};
    }
    if (value.isList()) {
        auto result = json::JsonArray{};
        list(value).forEach([&result](const Value &item) -> void { result.append(toJson(item)); });
        return json::JsonValue{std::move(result)};
    }
    if (value.isMap()) {
        auto result = json::JsonObject{};
        for (const auto &[key, item] : map(value)) {
            result.set(key, toJson(item));
        }
        return json::JsonValue{std::move(result)};
    }
    throw err::ParameterError{"Callbacks cannot be converted to JSON."_el, "value"_el};
}

auto capitalize(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    if (!value.isText()) {
        throw err::ParameterError{"The capitalize filter requires text."_el, "value"_el};
    }
    const auto text = value.asText().transformed(Char::toLowercase);
    if (text.isEmpty()) {
        return Value{text};
    }
    auto replacement = StringEditor{};
    replacement.append(text.charAt(unit::CpIndex::zero()).toUppercase());
    return Value{text.replaced(unit::CpRange{unit::CpIndex::zero(), unit::CpLength::one()}, String{replacement})};
}

auto lower(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    if (!value.isText()) {
        throw err::ParameterError{"The lower filter requires text."_el, "value"_el};
    }
    return Value{value.asText().transformed(Char::toLowercase)};
}

auto upper(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    if (!value.isText()) {
        throw err::ParameterError{"The upper filter requires text."_el, "value"_el};
    }
    return Value{value.asText().transformed(Char::toUppercase)};
}

auto trim(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 1U);
    if (!value.isText()) {
        throw err::ParameterError{"The trim filter requires text."_el, "value"_el};
    }
    if (arguments.isEmpty()) {
        return Value{value.asText().trimmed()};
    }
    const auto characters = argument(arguments, 0U);
    if (!characters.isText()) {
        throw err::ParameterError{"The trim characters must be text."_el, "arguments"_el};
    }
    return Value{value.asText().trimmed(CharSet{characters.asText()})};
}

auto replace(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 2U, 2U);
    if (!value.isText() || !argument(arguments, 0U).isText() || !argument(arguments, 1U).isText()) {
        throw err::ParameterError{"The replace filter requires text operands."_el, "value"_el};
    }
    return Value{value.asText().replacedAll(argument(arguments, 0U).asText(), argument(arguments, 1U).asText())};
}

auto first(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    if (value.isList()) {
        return list(value).get(unit::ItemIndex::zero(), Value{});
    }
    if (value.isText()) {
        return value.asText().isEmpty() ? Value{}
                                        : Value{value.asText().slice(StringSide::Front, unit::CpLength::one())};
    }
    throw err::ParameterError{"The first filter requires a list or text."_el, "value"_el};
}

auto last(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    if (value.isList()) {
        const auto count = list(value).count().toSizeT();
        return count == 0U ? Value{} : list(value).get(unit::ItemIndex::fromSizeT(count - 1U));
    }
    if (value.isText()) {
        return value.asText().isEmpty() ? Value{}
                                        : Value{value.asText().slice(StringSide::Back, unit::CpLength::one())};
    }
    throw err::ParameterError{"The last filter requires a list or text."_el, "value"_el};
}

auto join(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 2U);
    const auto separator = arguments.isEmpty() ? String{} : scalarText(argument(arguments, 0U));
    auto result = StringList{};
    const auto hasMember = arguments.count().toSizeT() == 2U;
    auto member = String{};
    if (hasMember) {
        const auto memberValue = argument(arguments, 1U);
        if (!memberValue.isText())
            throw err::ParameterError{"The join member must be text."_el, "arguments"_el};
        member = memberValue.asText();
    }
    list(value).forEach(
        [&](const Value &item) -> void { result.append(scalarText(hasMember ? item.get(member) : item)); });
    return Value{result.join(separator)};
}

auto length(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    if (value.isText()) {
        return Value{value.asText().characterLength().toSizeT()};
    }
    if (value.isList() || value.isMap()) {
        return Value{value.itemCount().toSizeT()};
    }
    throw err::ParameterError{"The length filter requires text, a list, or a map."_el, "value"_el};
}

auto reverse(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    if (value.isList()) {
        return Value{list(value).reversed()};
    }
    if (value.isText()) {
        const auto &text = value.asText();
        auto result = StringEditor{};
        result.reserve(text.length());
        auto index = unit::ByteIndex::end(text.length());
        while (index > unit::ByteIndex::zero()) {
            result.append(text.readCharAndRetreat(index));
        }
        return Value{String{result}};
    }
    throw err::ParameterError{"The reverse filter requires text or a list."_el, "value"_el};
}

auto sort(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 2U);
    auto reverseOrder = false;
    auto caseSensitive = false;
    if (!arguments.isEmpty()) {
        if (!argument(arguments, 0U).isBoolean())
            throw err::ParameterError{"The sort reverse flag must be boolean."_el, "arguments"_el};
        reverseOrder = argument(arguments, 0U).asBoolean();
    }
    if (arguments.count().toSizeT() == 2U) {
        if (!argument(arguments, 1U).isBoolean())
            throw err::ParameterError{"The sort case flag must be boolean."_el, "arguments"_el};
        caseSensitive = argument(arguments, 1U).asBoolean();
    }
    auto result = list(value).sorted(
        [caseSensitive](const Value &left, const Value &right) -> bool { return less(left, right, caseSensitive); });
    if (reverseOrder) {
        result.reverse();
    }
    return Value{result};
}

auto keys(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    auto result = ValueList{};
    for (const auto &entry : map(value)) {
        result.append(Value{entry.first});
    }
    return Value{result};
}

auto values(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    auto result = ValueList{};
    for (const auto &entry : map(value)) {
        result.append(entry.second);
    }
    return Value{result};
}

auto items(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    auto result = ValueList{};
    for (const auto &[key, item] : map(value)) {
        auto pair = ValueList{};
        pair.append(Value{key});
        pair.append(item);
        result.append(Value{pair});
    }
    return Value{result};
}

auto absolute(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    if (value.isInteger()) {
        return Value{
            value.asInteger() < 0 ? math::saturatingSubtract(int64_t{0}, value.asInteger()) : value.asInteger()};
    }
    if (value.isFloat()) {
        return Value{std::abs(value.asFloat())};
    }
    throw err::ParameterError{"The abs filter requires a number."_el, "value"_el};
}

auto round(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 2U);
    if (!value.isInteger() && !value.isFloat()) {
        throw err::ParameterError{"The round filter requires a number."_el, "value"_el};
    }
    auto precision = int64_t{};
    if (!arguments.isEmpty()) {
        if (!argument(arguments, 0U).isInteger()) {
            throw err::ParameterError{"The round precision must be an integer."_el, "arguments"_el};
        }
        precision = argument(arguments, 0U).asInteger();
        if (precision < -16 || precision > 16) {
            throw err::ParameterError{"The round precision must be between -16 and 16."_el, "arguments"_el};
        }
    }
    auto mode = String{"common"_el};
    if (arguments.count().toSizeT() == 2U) {
        if (!argument(arguments, 1U).isText()) {
            throw err::ParameterError{"The round mode must be text."_el, "arguments"_el};
        }
        mode = argument(arguments, 1U).asText();
    }
    if (mode != "common"_el && mode != "ceil"_el && mode != "floor"_el) {
        throw err::ParameterError{"The round mode must be common, ceil, or floor."_el, "arguments"_el};
    }
    const auto number = value.isInteger() ? static_cast<double>(value.asInteger()) : value.asFloat();
    const auto factor = std::pow(10.0, static_cast<double>(precision));
    const auto scaled = number * factor;
    const auto rounded =
        mode == "common"_el ? std::round(scaled) : (mode == "ceil"_el ? std::ceil(scaled) : std::floor(scaled));
    return Value{rounded / factor};
}

auto sum(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 1U);
    auto result = arguments.isEmpty() ? Value{int64_t{0}} : argument(arguments, 0U);
    if (!result.isInteger() && !result.isFloat()) {
        throw err::ParameterError{"The sum start must be numeric."_el, "arguments"_el};
    }
    list(value).forEach([&result](const Value &item) -> void {
        if (!item.isInteger() && !item.isFloat()) {
            throw err::ParameterError{"The sum list must contain only numbers."_el, "value"_el};
        }
        if (result.isInteger() && item.isInteger()) {
            result = Value{math::saturatingAdd(result.asInteger(), item.asInteger())};
        } else {
            const auto left = result.isInteger() ? static_cast<double>(result.asInteger()) : result.asFloat();
            const auto right = item.isInteger() ? static_cast<double>(item.asInteger()) : item.asFloat();
            result = Value{left + right};
        }
    });
    return result;
}

auto minimum(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    const auto &source = list(value);
    if (source.isEmpty()) {
        return {};
    }
    auto result = source.get(unit::ItemIndex::zero());
    source.forEach([&result](const Value &item) -> void {
        if (less(item, result, true)) {
            result = item;
        }
    });
    return result;
}

auto maximum(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    const auto &source = list(value);
    if (source.isEmpty()) {
        return {};
    }
    auto result = source.get(unit::ItemIndex::zero());
    source.forEach([&result](const Value &item) -> void {
        if (less(result, item, true)) {
            result = item;
        }
    });
    return result;
}

auto defaultValue(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 2U);
    const auto replacement = arguments.isEmpty() ? Value{String{}} : argument(arguments, 0U);
    auto useFalse = false;
    if (arguments.count().toSizeT() == 2U) {
        if (!argument(arguments, 1U).isBoolean()) {
            throw err::ParameterError{"The default false flag must be boolean."_el, "arguments"_el};
        }
        useFalse = argument(arguments, 1U).asBoolean();
    }
    return value.isNull() || (useFalse && !value.isTruthy()) ? replacement : value;
}

auto toJsonText(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 1U);
    auto indentation = int64_t{};
    if (!arguments.isEmpty()) {
        if (!argument(arguments, 0U).isInteger()) {
            throw err::ParameterError{"The JSON indentation must be an integer."_el, "arguments"_el};
        }
        indentation = argument(arguments, 0U).asInteger();
        if (indentation < 0 || indentation > 16) {
            throw err::ParameterError{"The JSON indentation must be between zero and 16."_el, "arguments"_el};
        }
    }
    auto options = json::JsonFormatOptions{};
    options.setIndentation(unit::CpLength{static_cast<uint32_t>(indentation)});
    return Value{toJson(value).toString(options)};
}
auto apply(const BuiltInFilter filter, const Value &value, const ValueList &arguments) -> Value {
    switch (filter) {
    case BuiltInFilter::Capitalize:
        return capitalize(value, arguments);
    case BuiltInFilter::Lower:
        return lower(value, arguments);
    case BuiltInFilter::Upper:
        return upper(value, arguments);
    case BuiltInFilter::Trim:
        return trim(value, arguments);
    case BuiltInFilter::Replace:
        return replace(value, arguments);
    case BuiltInFilter::First:
        return first(value, arguments);
    case BuiltInFilter::Last:
        return last(value, arguments);
    case BuiltInFilter::Join:
        return join(value, arguments);
    case BuiltInFilter::Length:
        return length(value, arguments);
    case BuiltInFilter::Reverse:
        return reverse(value, arguments);
    case BuiltInFilter::Sort:
        return sort(value, arguments);
    case BuiltInFilter::Keys:
        return keys(value, arguments);
    case BuiltInFilter::Values:
        return values(value, arguments);
    case BuiltInFilter::Items:
        return items(value, arguments);
    case BuiltInFilter::Absolute:
        return absolute(value, arguments);
    case BuiltInFilter::Round:
        return round(value, arguments);
    case BuiltInFilter::Sum:
        return sum(value, arguments);
    case BuiltInFilter::Minimum:
        return minimum(value, arguments);
    case BuiltInFilter::Maximum:
        return maximum(value, arguments);
    case BuiltInFilter::Default:
        return defaultValue(value, arguments);
    case BuiltInFilter::ToJson:
        return toJsonText(value, arguments);
    case BuiltInFilter::Count:
        break;
    }
    throw err::ParameterError{"The built-in filter identifier is invalid."_el, "filter"_el};
}

}
