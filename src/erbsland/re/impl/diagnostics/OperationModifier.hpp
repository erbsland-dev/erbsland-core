// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/Literals.hpp"
#include "../../../text/StdFormatForText.hpp"
#include "../../../text/StringView.hpp"

#include <cstdint>
#include <format>

namespace erbsland::re::impl {

enum class OperationModifier : uint8_t { Negated, Assert, CaseInsensitive, Skip, Add, Start, Stop };

[[nodiscard]] inline auto toString(const OperationModifier modifier) -> text::StringView {
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

template <>
struct std::formatter<erbsland::re::impl::OperationModifier> : std::formatter<erbsland::text::StringView> {
    auto format(const erbsland::re::impl::OperationModifier modifier, std::format_context &ctx) const {
        return std::formatter<erbsland::text::StringView>::format(erbsland::re::impl::toString(modifier), ctx);
    }
};
