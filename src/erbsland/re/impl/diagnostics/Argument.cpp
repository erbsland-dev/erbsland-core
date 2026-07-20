// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Argument.hpp"

#include "../../../text/Literals.hpp"

namespace erbsland::re::impl {

using namespace text::literals;

auto toString(ArgumentKind argumentKind) noexcept -> text::String {
    switch (argumentKind) {
    case ArgumentKind::Unknown:
        return "Unknown"_el;
    case ArgumentKind::ProgramCounter:
        return "Program Counter"_el;
    case ArgumentKind::Char:
        return "Char"_el;
    case ArgumentKind::CaptureGroup:
        return "Capture Group"_el;
    case ArgumentKind::Anchor:
        return "Anchor"_el;
    case ArgumentKind::Category:
        return "Category"_el;
    case ArgumentKind::SequenceIndex:
        return "Sequence Index"_el;
    case ArgumentKind::SequenceLength:
        return "Sequence Length"_el;
    case ArgumentKind::CharClassIndex:
        return "Char Class Index"_el;
    case ArgumentKind::CounterIndex:
        return "Counter Index"_el;
    case ArgumentKind::CounterValue:
        return "Counter Value"_el;
    case ArgumentKind::AtomicGroupId:
        return "Atomic Group ID"_el;
    }
    return {};
}

auto toString(ArgumentType argumentType) noexcept -> text::String {
    switch (argumentType) {
    case ArgumentType::Text:
        return "Text"_el;
    case ArgumentType::Integer:
        return "Integer"_el;
    case ArgumentType::Boolean:
        return "Boolean"_el;
    }
    return {};
}

auto argumentTypeFromValue(const ArgumentValue &value) noexcept -> ArgumentType {
    return std::visit(
        []<typename T>(const T &) -> ArgumentType {
            if constexpr (std::is_same_v<T, text::StringEditor>) {
                return ArgumentType::Text;
            } else if constexpr (std::is_same_v<T, uint32_t>) {
                return ArgumentType::Integer;
            } else if constexpr (std::is_same_v<T, bool>) {
                return ArgumentType::Boolean;
            }
        },
        value);
}

}
