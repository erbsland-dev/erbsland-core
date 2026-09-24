// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PlaceholderSource_fwd.hpp"

#include "../text/String.hpp"
#include "../text/StringList.hpp"

namespace erbsland::conf {

/// A provider for one or more placeholder sources.
/// @seedoc{/topics/conf/placeholders}
/// @tested{ParserPlaceholderTest}
class PlaceholderSource {
public:
    // defaults
    virtual ~PlaceholderSource() = default;

public:
    /// Get all source names implemented by this provider.
    /// @return A non-empty list of unique regular ELCL names.
    [[nodiscard]] virtual auto sourceNames() const -> text::StringList = 0;
    /// Resolve a placeholder source.
    /// @param sourceName The normalized, case-insensitive source name.
    /// @param parameter The decoded, case-preserving source parameter.
    /// @return The replacement text.
    /// @throws ConfError If the source cannot resolve the value.
    [[nodiscard]] virtual auto resolve(const text::String &sourceName, const text::String &parameter)
        -> text::String = 0;
};

}
