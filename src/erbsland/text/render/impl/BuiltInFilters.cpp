// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BuiltInFilters.hpp"

#include "BuiltInFilterError.hpp"
#include "ValueComparison.hpp"

#include "../Value.hpp"

#include "../../../data/json/JsonContainers.hpp"
#include "../../../data/json/JsonFormatOptions.hpp"
#include "../../../data/json/JsonValue.hpp"
#include "../../../err/ParameterFailure.hpp"
#include "../../../math/SaturatingMath.hpp"
#include "../../../unit/ItemCount.hpp"
#include "../../CharSet.hpp"
#include "../../Literals.hpp"
#include "../../StringEditor.hpp"
#include "../../StringList.hpp"
#include "../../StringSide.hpp"
#include "../../StringTransformations.hpp"

#include <cmath>

namespace erbsland::text::render::impl::built_in_filters {

using namespace unit;
using namespace text::literals;

void requireArity(const ValueList &arguments, const std::size_t minimum, const std::size_t maximum) {
    const auto count = arguments.count().toSizeT();
    if (count < minimum || count > maximum) {
        throwInputError("The built-in filter received an invalid number of arguments."_el);
    }
}

auto argument(const ValueList &arguments, const std::size_t index) -> const Value & {
    return arguments.getRefOrThrow(ItemIndex::fromSizeT(index));
}

auto list(const Value &value) -> const ValueList & {
    if (value.isList()) {
        return value.asList();
    }
    throwInputError("The built-in filter requires a list value."_el);
}

auto map(const Value &value) -> const ValueMap & {
    if (value.isMap()) {
        return value.asMap();
    }
    throwInputError("The built-in filter requires a map value."_el);
}

auto scalarText(const Value &value) -> String {
    if (value.isList() || value.isMap() || value.isCallback()) {
        throwInputError("The built-in filter requires a scalar value."_el);
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
        return toLowercase(left.asText()) < toLowercase(right.asText());
    }
    throwInputError("The values cannot be ordered by this built-in filter."_el);
}

auto toJson(const Value &value) -> data::json::JsonValue {
    if (value.isNull()) {
        return {};
    }
    if (value.isBoolean()) {
        return data::json::JsonValue{value.asBoolean()};
    }
    if (value.isInteger()) {
        return data::json::JsonValue{value.asInteger()};
    }
    if (value.isFloat()) {
        return data::json::JsonValue{value.asFloat()};
    }
    if (value.isText()) {
        return data::json::JsonValue{value.asText()};
    }
    if (value.isList()) {
        auto result = data::json::JsonArray{};
        list(value).forEach([&result](const Value &item) -> void { result.append(toJson(item)); });
        return data::json::JsonValue{std::move(result)};
    }
    if (value.isMap()) {
        auto result = data::json::JsonObject{};
        for (const auto &[key, item] : map(value)) {
            result.set(key, toJson(item));
        }
        return data::json::JsonValue{std::move(result)};
    }
    throwInputError("Callbacks cannot be converted to JSON."_el);
}

auto capitalize(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    if (!value.isText()) {
        throwInputError("The capitalize filter requires text."_el);
    }
    return Value{toCapitalized(value.asText())};
}

auto lower(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    if (!value.isText()) {
        throwInputError("The lower filter requires text."_el);
    }
    return Value{toLowercase(value.asText())};
}

auto upper(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    if (!value.isText()) {
        throwInputError("The upper filter requires text."_el);
    }
    return Value{toUppercase(value.asText())};
}

auto trim(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 1U);
    if (!value.isText()) {
        throwInputError("The trim filter requires text."_el);
    }
    if (arguments.isEmpty()) {
        return Value{value.asText().trimmed()};
    }
    const auto characters = argument(arguments, 0U);
    if (!characters.isText()) {
        throwInputError("The trim characters must be text."_el);
    }
    return Value{value.asText().trimmed(CharSet{characters.asText()})};
}

auto replace(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 2U, 2U);
    if (!value.isText() || !argument(arguments, 0U).isText() || !argument(arguments, 1U).isText()) {
        throwInputError("The replace filter requires text operands."_el);
    }
    return Value{value.asText().replacedAll(argument(arguments, 0U).asText(), argument(arguments, 1U).asText())};
}

auto first(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    if (value.isList()) {
        return list(value).get(ItemIndex::zero(), Value{});
    }
    if (value.isText()) {
        return value.asText().isEmpty() ? Value{} : Value{value.asText().slice(StringSide::Front, CpCount::one())};
    }
    throwInputError("The first filter requires a list or text."_el);
}

auto last(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    if (value.isList()) {
        const auto count = list(value).count().toSizeT();
        return count == 0U ? Value{} : list(value).get(ItemIndex::fromSizeT(count - 1U));
    }
    if (value.isText()) {
        return value.asText().isEmpty() ? Value{} : Value{value.asText().slice(StringSide::Back, CpCount::one())};
    }
    throwInputError("The last filter requires a list or text."_el);
}

auto join(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 2U);
    const auto separator = arguments.isEmpty() ? String{} : scalarText(argument(arguments, 0U));
    auto result = StringList{};
    const auto hasMember = arguments.count() == ItemCount{2U};
    auto member = String{};
    if (hasMember) {
        const auto memberValue = argument(arguments, 1U);
        if (!memberValue.isText()) {
            throwInputError("The join member must be text."_el);
        }
        member = memberValue.asText();
    }
    list(value).forEach(
        [&](const Value &item) -> void { result.append(scalarText(hasMember ? item.get(member) : item)); });
    return Value{result.join(separator)};
}

auto batch(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 1U, 2U);
    const auto &source = list(value);
    const auto &sizeValue = argument(arguments, 0U);
    if (!sizeValue.isInteger() || sizeValue.asInteger() <= 0) {
        throwInputError("The batch size must be a positive integer."_el);
    }
    const auto batchSize = ItemCount{static_cast<uint64_t>(sizeValue.asInteger()), ItemCount::Unchecked{}};
    const auto fill = arguments.count() == ItemCount{2U} ? argument(arguments, 1U) : Value{};
    auto result = ValueList{};
    auto row = ValueList{};
    source.forEach([&](const Value &item) -> void {
        row.append(item);
        if (row.count() == batchSize) {
            result.append(Value{row});
            row = ValueList{};
        }
    });
    if (!row.isEmpty()) {
        if (!fill.isNull()) {
            row.resize(batchSize, fill);
        }
        result.append(Value{row});
    }
    return Value{result};
}

auto length(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 0U);
    if (value.isText()) {
        return Value{value.asText().characterLength().toSizeT()};
    }
    if (value.isList() || value.isMap()) {
        return Value{value.itemCount().toSizeT()};
    }
    throwInputError("The length filter requires text, a list, or a map."_el);
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
        auto index = ByteIndex::end(text.length());
        while (index > ByteIndex::zero()) {
            result.append(text.readCharAndRetreat(index));
        }
        return Value{result};
    }
    throwInputError("The reverse filter requires text or a list."_el);
}

auto sort(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 2U);
    auto reverseOrder = false;
    auto caseSensitive = false;
    if (!arguments.isEmpty()) {
        if (!argument(arguments, 0U).isBoolean()) {
            throwInputError("The sort reverse flag must be boolean."_el);
        }
        reverseOrder = argument(arguments, 0U).asBoolean();
    }
    if (arguments.count() == ItemCount{2U}) {
        if (!argument(arguments, 1U).isBoolean()) {
            throwInputError("The sort case flag must be boolean."_el);
        }
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
    throwInputError("The abs filter requires a number."_el);
}

auto round(const Value &value, const ValueList &arguments) -> Value {
    requireArity(arguments, 0U, 2U);
    if (!value.isInteger() && !value.isFloat()) {
        throwInputError("The round filter requires a number."_el);
    }
    auto precision = int64_t{};
    if (!arguments.isEmpty()) {
        if (!argument(arguments, 0U).isInteger()) {
            throwInputError("The round precision must be an integer."_el);
        }
        precision = argument(arguments, 0U).asInteger();
        if (precision < -16 || precision > 16) {
            throwInputError("The round precision must be between -16 and 16."_el);
        }
    }
    auto mode = String{"common"_el};
    if (arguments.count() == ItemCount{2U}) {
        if (!argument(arguments, 1U).isText()) {
            throwInputError("The round mode must be text."_el);
        }
        mode = argument(arguments, 1U).asText();
    }
    if (mode != "common"_el && mode != "ceil"_el && mode != "floor"_el) {
        throwInputError("The round mode must be common, ceil, or floor."_el);
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
        throwInputError("The sum start must be numeric."_el);
    }
    list(value).forEach([&result](const Value &item) -> void {
        if (!item.isInteger() && !item.isFloat()) {
            throwInputError("The sum list must contain only numbers."_el);
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
    auto result = source.get(ItemIndex::zero());
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
    auto result = source.get(ItemIndex::zero());
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
    if (arguments.count() == ItemCount{2U}) {
        if (!argument(arguments, 1U).isBoolean()) {
            throwInputError("The default false flag must be boolean."_el);
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
            throwInputError("The JSON indentation must be an integer."_el);
        }
        indentation = argument(arguments, 0U).asInteger();
        if (indentation < 0 || indentation > 16) {
            throwInputError("The JSON indentation must be between zero and 16."_el);
        }
    }
    auto options = data::json::JsonFormatOptions{};
    options.setIndentation(CpCount{static_cast<uint32_t>(indentation), CpCount::Unchecked{}});
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
    case BuiltInFilter::Batch:
        return batch(value, arguments);
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
    throw err::ParameterFailure{"The built-in filter identifier is invalid."_el, "filter"_el};
}

}
