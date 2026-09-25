// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Source_fwd.hpp"

#include "../String.hpp"
#include "../StringList.hpp"

namespace erbsland::text::placeholder {

/// A provider for one or more placeholder sources.
/// @seedoc{/topics/text_placeholders/placeholders}
/// @tested{ReplacerTest ParserPlaceholderTest}
class Source {
public:
    // defaults
    virtual ~Source() = default;

public:
    /// Get all source names implemented by this provider.
    /// @return A non-empty list of unique regular ELCL names.
    [[nodiscard]] virtual auto sourceNames() const -> StringList = 0;
    /// Resolve a placeholder source.
    /// @param sourceName The normalized, case-insensitive source name.
    /// @param parameter The decoded, case-preserving source parameter.
    /// @return The replacement text.
    /// @throws ReplacerError If the source cannot resolve the value.
    [[nodiscard]] virtual auto resolve(const String &sourceName, const String &parameter) -> String = 0;
    /// Validate a source name and parameter without producing a replacement.
    /// The default calls `resolve()` and returns false if it throws `ReplacerError`.
    /// @return `true` when this source accepts the expression.
    [[nodiscard]] virtual auto validate(const String &sourceName, const String &parameter) -> bool;
};

}
