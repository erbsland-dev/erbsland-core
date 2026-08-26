// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Engine.hpp"

#include "BuiltInFilters.hpp"
#include "CompiledBlock.hpp"
#include "CompiledExtends.hpp"
#include "CompiledInclude.hpp"
#include "CompiledLayout.hpp"
#include "ProgramError.hpp"
#include "ValueComparison.hpp"
#include "ValueData.hpp"
#include "ValueTest.hpp"

#include "../../../math/SaturatingMath.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringFormat.hpp"

#include <algorithm>
#include <bit>
#include <cmath>

namespace erbsland::text::render::impl {

using namespace text::literals;

void Engine::executeUnary(ExecutionFrame &frame, const Opcode opcode, const unit::CodeLocation location) {
    auto &value = resolvedTop(frame, location);
    if (!value.isInteger() && !value.isFloat()) {
        throw error(
            RenderErrorCategory::Runtime,
            "Unary arithmetic requires a number"_el,
            "Unary '+' and '-' accept only integer or floating-point values."_el,
            frame.layout,
            location);
    }
    if (opcode == Opcode::UnaryPlus) {
        return;
    }
    if (opcode != Opcode::UnaryMinus) {
        throw ProgramError{"A non-unary opcode reached unary execution."_el};
    }
    value =
        value.isInteger() ? Value{math::saturatingSubtract(int64_t{0}, value.asInteger())} : Value{-value.asFloat()};
}

void Engine::executeArithmetic(ExecutionFrame &frame, const Opcode opcode, const unit::CodeLocation location) {
    auto right = takeValue(frame, opcode, location);
    auto left = takeValue(frame, opcode, location);
    const auto leftNumeric = left.isInteger() || left.isFloat();
    const auto rightNumeric = right.isInteger() || right.isFloat();
    if (!leftNumeric || !rightNumeric) {
        throw error(
            RenderErrorCategory::Runtime,
            "Arithmetic requires numbers"_el,
            "Binary arithmetic accepts only integer or floating-point operands."_el,
            frame.layout,
            location);
    }
    if (opcode == Opcode::Divide &&
        ((right.isInteger() && right.asInteger() == 0) || (right.isFloat() && right.asFloat() == 0.0))) {
        push(frame, Value{}, location);
        return;
    }
    if (left.isInteger() && right.isInteger()) {
        auto result = int64_t{};
        switch (opcode) {
        case Opcode::Add:
            result = math::saturatingAdd(left.asInteger(), right.asInteger());
            break;
        case Opcode::Subtract:
            result = math::saturatingSubtract(left.asInteger(), right.asInteger());
            break;
        case Opcode::Multiply:
            result = math::saturatingMultiply(left.asInteger(), right.asInteger());
            break;
        case Opcode::Divide:
            result = math::saturatingDivide(left.asInteger(), right.asInteger());
            break;
        default:
            throw ProgramError{"A non-arithmetic opcode reached arithmetic execution."_el};
        }
        push(frame, Value{result}, location);
        return;
    }
    const auto leftValue = left.isInteger() ? static_cast<double>(left.asInteger()) : left.asFloat();
    const auto rightValue = right.isInteger() ? static_cast<double>(right.asInteger()) : right.asFloat();
    auto result = double{};
    switch (opcode) {
    case Opcode::Add:
        result = leftValue + rightValue;
        break;
    case Opcode::Subtract:
        result = leftValue - rightValue;
        break;
    case Opcode::Multiply:
        result = leftValue * rightValue;
        break;
    case Opcode::Divide:
        result = leftValue / rightValue;
        break;
    default:
        throw ProgramError{"A non-arithmetic opcode reached arithmetic execution."_el};
    }
    push(frame, Value{result}, location);
}

void Engine::executeConcatenation(ExecutionFrame &frame, const unit::CodeLocation location) {
    auto right = takeValue(frame, Opcode::Concatenate, location);
    auto left = takeValue(frame, Opcode::Concatenate, location);
    if (left.isList() || left.isMap() || right.isList() || right.isMap()) {
        throw error(
            RenderErrorCategory::Runtime,
            "Container concatenation is not supported"_el,
            "The '~' operator accepts only null and scalar operands."_el,
            frame.layout,
            location);
    }
    auto parts = StringList{};
    parts.append(left.toString());
    parts.append(right.toString());
    push(frame, Value{parts.join()}, location);
}

void Engine::buildCollection(
    ExecutionFrame &frame, const Opcode opcode, const uint64_t count, const unit::CodeLocation location) {
    if (count > frame.stack.size() || (opcode == Opcode::BuildMap && count > frame.stack.size() / 2U)) {
        throw ProgramError{"A collection literal consumes more values than are available on the stack."_el};
    }
    if (opcode == Opcode::BuildList) {
        auto result = ValueList{};
        for (auto index = uint64_t{}; index < count; ++index) {
            result.prepend(takeValue(frame, opcode, location));
        }
        push(frame, Value{result}, location);
        return;
    }
    if (opcode != Opcode::BuildMap) {
        throw ProgramError{"A non-collection opcode reached collection construction."_el};
    }
    auto result = ValueMap{};
    for (auto index = uint64_t{}; index < count; ++index) {
        auto value = takeValue(frame, opcode, location);
        auto key = takeValue(frame, opcode, location);
        if (!key.isText() || result.contains(key.asText())) {
            throw ProgramError{"A compiled map literal has a non-text or duplicate key."_el};
        }
        result.set(key.asText(), std::move(value));
    }
    push(frame, Value{result}, location);
}

void Engine::executeMembership(ExecutionFrame &frame, const bool negate, const unit::CodeLocation location) {
    auto container = takeValue(frame, negate ? Opcode::NotIn : Opcode::In, location);
    auto needle = takeValue(frame, negate ? Opcode::NotIn : Opcode::In, location);
    if (needle.isList() || needle.isMap()) {
        throw error(
            RenderErrorCategory::Runtime,
            "Membership requires a scalar value"_el,
            "The left operand of 'in' must be null or a scalar."_el,
            frame.layout,
            location);
    }
    auto result = false;
    if (container.isText() && needle.isText()) {
        result = container.asText().contains(needle.asText());
    } else if (container.isList()) {
        const auto &values = list(container);
        for (auto index = std::size_t{}; index < values.count().toSizeT(); ++index) {
            if (valuesEqual(needle, values.get(unit::ItemIndex::fromSizeT(index)))) {
                result = true;
                break;
            }
        }
    } else if (container.isMap() && needle.isText()) {
        result = map(container).contains(needle.asText());
    } else {
        throw error(
            RenderErrorCategory::Runtime,
            "Unsupported membership operands"_el,
            "Membership supports text in text, a scalar in a list, or text keys in a map."_el,
            frame.layout,
            location);
    }
    push(frame, Value{negate ? !result : result}, location);
}

void Engine::executeTest(
    ExecutionFrame &frame, const uint64_t rawTest, const bool negate, const unit::CodeLocation location) {
    if (rawTest >= static_cast<uint64_t>(ValueTest::Count)) {
        throw ProgramError{"A value-test operand is invalid."_el};
    }
    const auto test = static_cast<ValueTest>(rawTest);
    auto value = takeValue(frame, negate ? Opcode::IsNot : Opcode::Is, location);
    auto result = false;
    switch (test) {
    case ValueTest::Null:
        result = value.isNull();
        break;
    case ValueTest::True:
        result = value.isBoolean() && value.asBoolean();
        break;
    case ValueTest::False:
        result = value.isBoolean() && !value.asBoolean();
        break;
    case ValueTest::Boolean:
        result = value.isBoolean();
        break;
    case ValueTest::Integer:
        result = value.isInteger();
        break;
    case ValueTest::Float:
        result = value.isFloat();
        break;
    case ValueTest::Number:
        result = value.isInteger() || value.isFloat();
        break;
    case ValueTest::Text:
        result = value.isText();
        break;
    case ValueTest::List:
        result = value.isList();
        break;
    case ValueTest::Map:
        result = value.isMap();
        break;
    case ValueTest::Iterable:
        result = value.isList() || value.isMap() || value.isText();
        break;
    case ValueTest::Scalar:
        result = !value.isList() && !value.isMap() && !value.isCallback();
        break;
    case ValueTest::Even:
        result = value.isInteger() && value.asInteger() % 2 == 0;
        break;
    case ValueTest::Odd:
        result = value.isInteger() && value.asInteger() % 2 != 0;
        break;
    case ValueTest::Count:
        throw ProgramError{"The value-test count sentinel cannot be executed."_el};
    }
    push(frame, Value{negate ? !result : result}, location);
}

void Engine::executeComparison(ExecutionFrame &frame, const Opcode opcode, const unit::CodeLocation location) {
    auto right = takeValue(frame, opcode, location);
    auto left = takeValue(frame, opcode, location);
    if (left.isList() || left.isMap() || right.isList() || right.isMap()) {
        throw error(
            RenderErrorCategory::Runtime,
            "Container comparison is not supported"_el,
            "Lists and maps cannot be compared in layout expressions."_el,
            frame.layout,
            location);
    }
    auto result = false;
    if (opcode == Opcode::Equal || opcode == Opcode::NotEqual) {
        result = valuesEqual(left, right);
        if (opcode == Opcode::NotEqual) {
            result = !result;
        }
    } else {
        const auto order = compareValues(frame.layout, left, right, location);
        switch (opcode) {
        case Opcode::Greater:
            result = order > 0;
            break;
        case Opcode::GreaterEqual:
            result = order >= 0;
            break;
        case Opcode::Less:
            result = order < 0;
            break;
        case Opcode::LessEqual:
            result = order <= 0;
            break;
        default:
            throw ProgramError{"A non-ordering opcode reached ordering comparison."_el};
        }
    }
    push(frame, Value{result}, location);
}

auto Engine::valuesEqual(const Value &left, const Value &right) const -> bool {
    if ((left.isInteger() || left.isFloat()) && (right.isInteger() || right.isFloat())) {
        return value_comparison::compareNumbers(left, right) == 0;
    }
    if (left.type() != right.type()) {
        return false;
    }
    if (left.isNull()) {
        return true;
    }
    if (left.isBoolean()) {
        return left.asBoolean() == right.asBoolean();
    }
    if (left.isText()) {
        return left.asText() == right.asText();
    }
    return false;
}

auto Engine::compareValues(
    const ConstCompiledLayoutPtr &layout,
    const Value &left,
    const Value &right,
    const unit::CodeLocation location) const -> std::partial_ordering {
    if ((left.isInteger() || left.isFloat()) && (right.isInteger() || right.isFloat())) {
        return value_comparison::compareNumbers(left, right);
    }
    if (left.isText() && right.isText()) {
        const auto result = left.asText() <=> right.asText();
        if (result < 0) {
            return std::partial_ordering::less;
        }
        if (result > 0) {
            return std::partial_ordering::greater;
        }
        return std::partial_ordering::equivalent;
    }
    throw error(
        RenderErrorCategory::Runtime,
        "Values cannot be ordered"_el,
        "Ordering comparisons require two numbers or two text values."_el,
        layout,
        location);
}

void Engine::applyFilter(
    ExecutionFrame &frame, const uint64_t operand, const bool builtIn, const unit::CodeLocation location) {
    const auto argumentCount = operand & 0x03U;
    const auto identifier = operand >> 2U;
    if (argumentCount > 2U) {
        throw ProgramError{"A filter instruction contains an invalid argument count."_el};
    }
    auto applicationFilter = static_cast<const FilterFn *>(nullptr);
    if (builtIn) {
        if (identifier >= static_cast<uint64_t>(BuiltInFilter::Count)) {
            throw ProgramError{"A compiled layout references an invalid built-in filter."_el};
        }
    } else {
        const auto name = constant(frame, identifier, "filter"_el);
        applicationFilter = frame.layout->applicationFilter(name);
        if (applicationFilter == nullptr || !*applicationFilter) {
            throw ProgramError{"A compiled layout references an unknown application filter."_el};
        }
    }
    auto arguments = ValueList{};
    for (auto argumentIndex = uint64_t{}; argumentIndex < argumentCount; ++argumentIndex) {
        arguments.prepend(
            takeValue(frame, builtIn ? Opcode::ApplyBuiltInFilter : Opcode::ApplyApplicationFilter, location));
    }
    auto &value = resolvedTop(frame, location);
    try {
        if (builtIn) {
            value = resolve(
                built_in_filters::apply(static_cast<BuiltInFilter>(identifier), value, arguments),
                frame.layout,
                location);
        } else {
            auto values = ValueList{};
            values.reserve(unit::ItemCount::fromSizeT(static_cast<std::size_t>(argumentCount) + 1U));
            values.append(value);
            values.append(arguments);
            value = resolve((*applicationFilter)(values), frame.layout, location);
        }
    } catch (const RenderError &) {
        throw;
    } catch (const ProgramError &) {
        throw;
    } catch (...) {
        throw error(
            RenderErrorCategory::Runtime,
            "Layout filter failed"_el,
            "A registered layout filter raised an exception."_el,
            frame.layout,
            location,
            std::current_exception());
    }
}

auto Engine::list(const Value &value) -> const ValueList & {
    if (!value.isList()) {
        throw ProgramError{"A non-list value was used as an internal list."_el};
    }
    return value.asList();
}

auto Engine::map(const Value &value) -> const ValueMap & {
    if (!value.isMap()) {
        throw ProgramError{"A non-map value was used as an internal map."_el};
    }
    return value.asMap();
}

}
