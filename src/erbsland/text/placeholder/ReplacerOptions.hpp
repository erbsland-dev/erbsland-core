// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EscapeMode.hpp"

#include "../String.hpp"

namespace erbsland::text::placeholder {

/// Syntax options for a standalone placeholder replacer.
/// @tested{ReplacerTest}
class ReplacerOptions final {
public:
    /// Create `${...}` syntax with `|` filters, `:` parameters, and backslash escaping.
    ReplacerOptions();

public:
    /// Set nonempty opening and closing frame strings, each at most 16 code points.
    auto setFrame(String begin, String end) -> ReplacerOptions &;
    /// Set the filter separator; empty disables filters.
    auto setFilterSeparator(String separator) -> ReplacerOptions &;
    /// Set the name/parameter separator; empty disables parameters.
    auto setNameSeparator(String separator) -> ReplacerOptions &;
    /// Set the delimiter escape strategy.
    auto setEscapeMode(EscapeMode mode) noexcept -> ReplacerOptions &;

    /// Get the opening frame.
    [[nodiscard]] auto frameBegin() const noexcept -> const String & { return _frameBegin; }
    /// Get the closing frame.
    [[nodiscard]] auto frameEnd() const noexcept -> const String & { return _frameEnd; }
    /// Get the filter separator.
    [[nodiscard]] auto filterSeparator() const noexcept -> const String & { return _filterSeparator; }
    /// Get the name/parameter separator.
    [[nodiscard]] auto nameSeparator() const noexcept -> const String & { return _nameSeparator; }
    /// Get the escape strategy.
    [[nodiscard]] auto escapeMode() const noexcept -> EscapeMode { return _escapeMode; }

    /// Validate cross-field escape constraints before constructing a replacer.
    void validate() const;

private:
    /// Verify a delimiter length before changing an option.
    static void verifyLength(const String &value, const String &name, bool allowEmpty);
    /// Test whether the first two code points of a delimiter match.
    [[nodiscard]] static auto hasDoubledStart(const String &value) noexcept -> bool;

private:
    String _frameBegin;                            ///< Opening frame string.
    String _frameEnd;                              ///< Closing frame string.
    String _filterSeparator;                       ///< Filter separator.
    String _nameSeparator;                         ///< Name/parameter separator.
    EscapeMode _escapeMode{EscapeMode::Backslash}; ///< Escape strategy.
};

}
