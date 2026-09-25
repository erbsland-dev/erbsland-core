// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Filter_fwd.hpp"

#include "../String.hpp"
#include "../StringList.hpp"

namespace erbsland::text::placeholder {

/// A provider for one or more placeholder filters.
/// @seedoc{/topics/text_placeholders/placeholders}
/// @tested{ReplacerTest ParserPlaceholderTest}
class Filter {
public:
    // defaults
    virtual ~Filter() = default;

public:
    /// Get all filter names implemented by this provider.
    /// @return A non-empty list of unique regular ELCL names.
    [[nodiscard]] virtual auto filterNames() const -> StringList = 0;
    /// Apply a filter to a placeholder value.
    /// @param filterName The normalized, case-insensitive filter name.
    /// @param parameter The decoded, case-preserving filter parameter.
    /// @param value The value from the source or preceding filter.
    /// @return The filtered replacement text.
    /// @throws ReplacerError If the value cannot be filtered.
    [[nodiscard]] virtual auto apply(const String &filterName, const String &parameter, const String &value)
        -> String = 0;
    /// Validate a filter name and parameter without a current value.
    /// The default calls `apply()` with an empty value and returns false if it throws `ReplacerError`.
    /// @return `true` when this filter accepts the expression.
    [[nodiscard]] virtual auto validate(const String &filterName, const String &parameter) -> bool;
};

}
