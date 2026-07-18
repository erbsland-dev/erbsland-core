// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OperationData.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::re::impl {

using namespace text::literals;

auto operationData() noexcept -> const std::vector<OperationData> & {
    static std::vector<OperationData> data = {
        {Operation::None, Operation::None, "NONE"_el, {}, "NONE"_el, {}},
        {Operation::Split,
            Operation::Split,
            "SPLIT"_el,
            {},
            "SPLIT"_el,
            {
                {ArgumentKind::ProgramCounter, ArgumentType::Integer},
                {ArgumentKind::ProgramCounter, ArgumentType::Integer},
            }},
        {Operation::Jump,
            Operation::Jump,
            "JUMP"_el,
            {},
            "JUMP"_el,
            {{ArgumentKind::ProgramCounter, ArgumentType::Integer}}},
        {Operation::Match, Operation::Match, "MATCH"_el, {}, "MATCH"_el, {}},
        {Operation::NotMatch, Operation::Match, "MATCH"_el, {OperationModifier::Negated}, "NOT MATCH"_el, {}},
        {Operation::Anchor,
            Operation::Anchor,
            "ANCHOR"_el,
            {},
            "ANCHOR"_el,
            {{ArgumentKind::Anchor, ArgumentType::Integer}}},
        {Operation::StartCapture,
            Operation::StartCapture,
            "CAPTURE"_el,
            {OperationModifier::Start},
            "START CAPTURE"_el,
            {{ArgumentKind::CaptureGroup, ArgumentType::Integer}}},
        {Operation::StopCapture,
            Operation::StartCapture,
            "CAPTURE"_el,
            {OperationModifier::Stop},
            "STOP CAPTURE"_el,
            {{ArgumentKind::CaptureGroup, ArgumentType::Integer}}},
        {Operation::StartAtomic,
            Operation::StartAtomic,
            "ATOMIC"_el,
            {OperationModifier::Start},
            "START ATOMIC"_el,
            {{ArgumentKind::AtomicGroupId, ArgumentType::Integer}}},
        {Operation::StopAtomic,
            Operation::StartAtomic,
            "ATOMIC"_el,
            {OperationModifier::Stop},
            "STOP ATOMIC"_el,
            {{ArgumentKind::AtomicGroupId, ArgumentType::Integer}}},
        {Operation::Counter,
            Operation::Counter,
            "COUNTER"_el,
            {},
            "COUNTER"_el,
            {
                {ArgumentKind::CounterIndex, ArgumentType::Integer},
                {ArgumentKind::CounterValue, ArgumentType::Integer},
            }},
        {Operation::AddCounter,
            Operation::Counter,
            "COUNTER"_el,
            {OperationModifier::Add},
            "ADD COUNTER"_el,
            {
                {ArgumentKind::CounterIndex, ArgumentType::Integer},
                {ArgumentKind::CounterValue, ArgumentType::Integer},
            }},
        {Operation::Maximum,
            Operation::Maximum,
            "MAXIMUM"_el,
            {},
            "MAXIMUM"_el,
            {
                {ArgumentKind::CounterIndex, ArgumentType::Integer},
                {ArgumentKind::CounterValue, ArgumentType::Integer},
            }},
        {Operation::SkipIfMaximum,
            Operation::Maximum,
            "MAXIMUM"_el,
            {OperationModifier::Skip},
            "SKIP MAXIMUM"_el,
            {
                {ArgumentKind::CounterIndex, ArgumentType::Integer},
                {ArgumentKind::CounterValue, ArgumentType::Integer},
            }},
        {Operation::Minimum,
            Operation::Minimum,
            "MINIMUM"_el,
            {},
            "MINIMUM"_el,
            {
                {ArgumentKind::CounterIndex, ArgumentType::Integer},
                {ArgumentKind::CounterValue, ArgumentType::Integer},
            }},
        {Operation::Success, Operation::Success, "SUCCESS"_el, {}, "SUCCESS"_el, {}},
        {Operation::Failure, Operation::Failure, "FAILURE"_el, {}, "FAILURE"_el, {}},
        {Operation::Char, Operation::Char, "CHAR"_el, {}, "CHAR"_el, {{ArgumentKind::Char, ArgumentType::Integer}}},
        {Operation::CiChar,
            Operation::Char,
            "CHAR"_el,
            {OperationModifier::CaseInsensitive},
            "CI CHAR"_el,
            {{ArgumentKind::Char, ArgumentType::Integer}}},
        {Operation::NotChar,
            Operation::Char,
            "CHAR"_el,
            {OperationModifier::Negated},
            "NOT CHAR"_el,
            {{ArgumentKind::Char, ArgumentType::Integer}}},
        {Operation::NotCiChar,
            Operation::Char,
            "CHAR"_el,
            {OperationModifier::CaseInsensitive, OperationModifier::Negated},
            "NOT CI CHAR"_el,
            {{ArgumentKind::Char, ArgumentType::Integer}}},
        {Operation::Sequence,
            Operation::Sequence,
            "SEQUENCE"_el,
            {},
            "SEQUENCE"_el,
            {
                {ArgumentKind::SequenceIndex, ArgumentType::Integer},
                {ArgumentKind::SequenceLength, ArgumentType::Integer},
            }},
        {Operation::CiSequence,
            Operation::Sequence,
            "SEQUENCE"_el,
            {OperationModifier::CaseInsensitive},
            "CI SEQUENCE"_el,
            {
                {ArgumentKind::SequenceIndex, ArgumentType::Integer},
                {ArgumentKind::SequenceLength, ArgumentType::Integer},
            }},
        {Operation::Category,
            Operation::Category,
            "CATEGORY"_el,
            {},
            "CATEGORY"_el,
            {{ArgumentKind::Category, ArgumentType::Integer}}},
        {Operation::NotCategory,
            Operation::Category,
            "CATEGORY"_el,
            {OperationModifier::Negated},
            "NOT CATEGORY"_el,
            {{ArgumentKind::Category, ArgumentType::Integer}}},
        {Operation::AssertCategory,
            Operation::Category,
            "CATEGORY"_el,
            {OperationModifier::Assert},
            "ASSERT CATEGORY"_el,
            {{ArgumentKind::Category, ArgumentType::Integer}}},
        {Operation::NotAssertCategory,
            Operation::Category,
            "CATEGORY"_el,
            {OperationModifier::Negated, OperationModifier::Assert},
            "NOT ASSERT CATEGORY"_el,
            {{ArgumentKind::Category, ArgumentType::Integer}}},
        {Operation::Class,
            Operation::Class,
            "CLASS"_el,
            {},
            "CLASS"_el,
            {{ArgumentKind::CharClassIndex, ArgumentType::Integer}}},
        {Operation::CiClass,
            Operation::Class,
            "CLASS"_el,
            {OperationModifier::CaseInsensitive},
            "CI CLASS"_el,
            {{ArgumentKind::CharClassIndex, ArgumentType::Integer}}},
        {Operation::NotClass,
            Operation::Class,
            "CLASS"_el,
            {OperationModifier::Negated},
            "NOT CLASS"_el,
            {{ArgumentKind::CharClassIndex, ArgumentType::Integer}}},
        {Operation::NotCiClass,
            Operation::Class,
            "CLASS"_el,
            {OperationModifier::CaseInsensitive, OperationModifier::Negated},
            "NOT CI CLASS"_el,
            {{ArgumentKind::CharClassIndex, ArgumentType::Integer}}},
        {Operation::Any, Operation::Any, "ANY"_el, {}, "ANY"_el, {}},
        {Operation::Unknown, Operation::Unknown, "???"_el, {}, "???"_el, {}}};
    return data;
}

auto dataForOperation(const Operation operation) noexcept -> const OperationData & {
    for (const auto &data : operationData()) {
        if (data.operation == operation) {
            return data;
        }
    }
    return operationData().back();
}

auto baseOperationForString(const text::String &baseName) -> Operation {
    for (const auto &data : operationData()) {
        if (baseName.compare(data.baseName, text::Char::compareCaseFolded) == std::strong_ordering::equal) {
            return data.operation;
        }
    }
    throw err::ParameterError{"Invalid operation name."_el, "str"_el};
}

auto modifiedOperation(const Operation baseOperation, const std::set<OperationModifier> &operationModifiers)
    -> Operation {

    for (const auto &data : operationData()) {
        if (data.baseOperation == baseOperation && data.modifiers == operationModifiers) {
            return data.operation;
        }
    }
    throw err::ParameterError{"Invalid operation modifiers."_el, "modifiers"_el};
}

auto toString(const Operation operation) noexcept -> text::String {
    return dataForOperation(operation).displayName;
}

auto toBaseName(const Operation operation) noexcept -> text::String {
    return dataForOperation(operation).baseName;
}

}
