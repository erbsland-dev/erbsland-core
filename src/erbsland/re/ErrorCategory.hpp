// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/Literals.hpp"
#include "../text/String.hpp"

#include <cstdint>

namespace erbsland::re {

/// The category of an `RegExError`.
enum class ErrorCategory : uint8_t {
    Parser,    ///< A parser error.
    Format,    ///< A format error in a replacement string.
    Assembler, ///< An assembler error.
    Engine,    ///< An engine error.
    Timeout,   ///< A timeout error.
    Limit,     ///< A limit error.
    Internal,  ///< An internal error.
};

/// Convert an error category into a stable, human-readable name.
[[nodiscard]] inline auto toString(const ErrorCategory category) noexcept -> text::String {
    using namespace text::literals;
    switch (category) {
    case ErrorCategory::Parser:
        return "Parser"_el;
    case ErrorCategory::Format:
        return "Format"_el;
    case ErrorCategory::Assembler:
        return "Assembler"_el;
    case ErrorCategory::Timeout:
        return "Timeout"_el;
    case ErrorCategory::Limit:
        return "Limit"_el;
    case ErrorCategory::Engine:
        return "Engine"_el;
    case ErrorCategory::Internal:
        return "Internal"_el;
    default:
        break;
    }
    return {};
}

}
