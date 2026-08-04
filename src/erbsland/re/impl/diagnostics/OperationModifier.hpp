// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/Literals.hpp"
#include "../../../text/String.hpp"

#include <cstdint>

namespace erbsland::re::impl {

/// Modifier for a regular-expression operation diagnostic.
enum class OperationModifier : uint8_t { Negated, Assert, CaseInsensitive, Skip, Add, Start, Stop };

/// Convert an operation modifier to display text.
[[nodiscard]] inline auto toString(const OperationModifier modifier) -> text::String {
    using namespace text::literals;
    switch (modifier) {
    case OperationModifier::Negated:
        return "NOT"_el;
    case OperationModifier::Assert:
        return "ASSERT"_el;
    case OperationModifier::CaseInsensitive:
        return "IS"_el;
    case OperationModifier::Skip:
        return "SKIP"_el;
    case OperationModifier::Add:
        return "ADD"_el;
    case OperationModifier::Start:
        return "START"_el;
    case OperationModifier::Stop:
        return "STOP"_el;
    }
    return "unknown"_el;
}

}
