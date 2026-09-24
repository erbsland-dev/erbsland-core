// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PlaceholderFilter_fwd.hpp"

#include "../text/String.hpp"
#include "../text/StringList.hpp"

namespace erbsland::conf {

/// A provider for one or more placeholder filters.
/// @seedoc{/topics/conf/placeholders}
/// @tested{ParserPlaceholderTest}
class PlaceholderFilter {
public:
    // defaults
    virtual ~PlaceholderFilter() = default;

public:
    /// Get all filter names implemented by this provider.
    /// @return A non-empty list of unique regular ELCL names.
    [[nodiscard]] virtual auto filterNames() const -> text::StringList = 0;
    /// Apply a filter to a placeholder value.
    /// @param filterName The normalized, case-insensitive filter name.
    /// @param parameter The decoded, case-preserving filter parameter.
    /// @param value The value from the source or preceding filter.
    /// @return The filtered replacement text.
    /// @throws ConfError If the value cannot be filtered.
    [[nodiscard]] virtual auto apply(
        const text::String &filterName, const text::String &parameter, const text::String &value) -> text::String = 0;
};

}
