// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EscapeAmount.hpp"

#include "Char.hpp"
#include "Literals.hpp"
#include "String.hpp"

#include "impl/ThrowHelper.hpp"

namespace erbsland::text {

using namespace text::literals;

auto EscapeAmount::toString() const -> StringView {
    switch (_value) {
    case Required:
        return "required"_el;
    case Balanced:
        return "balanced"_el;
    case NonAscii:
        return "non-ascii"_el;
    case Everything:
        return "all"_el;
    case Nothing:
    default:
        return "nothing"_el;
    }
}

auto EscapeAmount::fromString(const StringView &text) noexcept -> std::optional<EscapeAmount> {
    if (text == "nothing"_el) {
        return EscapeAmount{Nothing};
    }
    if (text == "required"_el) {
        return EscapeAmount{Required};
    }
    if (text == "balanced"_el) {
        return EscapeAmount{Balanced};
    }
    if (text == "non-ascii"_el) {
        return EscapeAmount{NonAscii};
    }
    if (text == "all"_el) {
        return EscapeAmount{Everything};
    }
    return {};
}

auto EscapeAmount::fromStringOrThrow(const StringView &text) -> EscapeAmount {
    if (const auto result = fromString(text); result.has_value()) {
        return result.value();
    }
    text::impl::throwParseError("Unsupported escape amount");
}

auto EscapeAmount::fromSuffix(const Char character) noexcept -> std::optional<EscapeAmount> {
    switch (character.toRawValue()) {
    case U'-':
        return EscapeAmount{Required};
    case U'=':
        return EscapeAmount{Balanced};
    case U'+':
        return EscapeAmount{NonAscii};
    case U'*':
        return EscapeAmount{Everything};
    default:
        return {};
    }
}

}
