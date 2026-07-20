// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EscapeFormatter.hpp"

namespace erbsland::text::impl {

/// Escape formatter for ELCL double-quoted text literals.
/// @tested{StringEscapingTest}
class ConfigEscapeFormatter final : public EscapeFormatter {
public:
    /// The format for the ELCL escaping.
    enum class Format : uint8_t {
        Regular, ///< The regular escaping format, for production.
        Test,    ///< The special test escaping format, according to the test specification.
    };

public:
    /// Create a new instance of the formatter.
    explicit ConfigEscapeFormatter(const Format format) : _format{format} {}

public: // implement EscapeFormatter
    [[nodiscard]] auto needsEscape(Char character, EscapeAmount amount) const noexcept -> bool override;
    void escape(Char character, AnyStringBuilder &builder) const override;
    [[nodiscard]] auto escapeSize(Char character, StringKind stringKind) const noexcept -> std::size_t override;

public:
    /// Create a new "regular" instance of the formatter.
    [[nodiscard]] static auto regularInstance() noexcept -> const EscapeFormatterPtr &;
    /// Create a new "test" instance of the formatter.
    [[nodiscard]] static auto testInstance() noexcept -> const EscapeFormatterPtr &;

private:
    [[nodiscard]] static auto unicodeEscapeFormat() noexcept -> IntegerFormat;
    [[nodiscard]] static auto hexadecimalDigitCount(Char character) noexcept -> std::size_t;

private:
    Format _format; ///< The format for the ELCL escaping.
};

}
